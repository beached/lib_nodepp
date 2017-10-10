// The MIT License (MIT)
//
// Copyright (c) 2014-2017 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <atomic>
#include <boost/any.hpp>
#include <boost/asio/error.hpp>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base_error.h"
#include <daw/daw_fixed_lookup.h>
#include <daw/daw_range_algorithm.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>

namespace daw {
	namespace nodepp {
		namespace base {
			template<typename Derived>
			struct enable_shared : public std::enable_shared_from_this<Derived> {
				std::shared_ptr<Derived> get_ptr( ) {
					return static_cast<Derived *>( this )->shared_from_this( );
				}
				std::weak_ptr<Derived> get_weak_ptr( ) {
					return this->get_ptr( );
				}

			protected:
				constexpr enable_shared( ) noexcept : std::enable_shared_from_this<Derived>{} {}
				enable_shared( enable_shared const & ) = default;
				constexpr enable_shared( enable_shared && ) noexcept = default;
				enable_shared &operator=( enable_shared const & ) = default;
				constexpr enable_shared &operator=( enable_shared && ) noexcept = default;

				~enable_shared( ) = default;
			}; // struct enable_shared

			namespace impl {
				constexpr size_t DefaultMaxEventCount = 100;
				template<size_t MaxEventCount = DefaultMaxEventCount>
				struct EventEmitterImpl;
			} // namespace impl

			using EventEmitter = std::shared_ptr<impl::EventEmitterImpl<impl::DefaultMaxEventCount>>;

			namespace impl {
				template<typename Listener, typename... ExpectedArgs>
				constexpr bool is_valid_listener_v = daw::is_callable_v<Listener, ExpectedArgs...>;

				template<typename Listener, typename... ExpectedArgs,
				         std::enable_if_t<daw::is_callable_v<Listener, ExpectedArgs...>, std::nullptr_t> = nullptr>
				constexpr auto make_listener_t( ) noexcept {
					struct result {
						size_t const arity = sizeof...( ExpectedArgs );

						std::function<void( ExpectedArgs... )> create( Listener listener ) const {
							return [listener = std::move( listener )]( ExpectedArgs... args ) mutable {
								listener( std::move( args )... );
							};
						}
					};
					return result{};
				};

				template<
				  typename Listener, typename... ExpectedArgs,
				  std::enable_if_t<sizeof...( ExpectedArgs ) != 0 && daw::is_callable_v<Listener>, std::nullptr_t> = nullptr>
				constexpr auto make_listener_t( ) noexcept {
					struct result {
						size_t const arity = 0;

						std::function<void( )> create( Listener listener ) const {
							return [listener = std::move( listener )]( ) mutable {
								listener( );
							};
						}
					};
					return result{};
				}

				struct callback_info_t {
					using callback_id_t = size_t;
					enum class run_mode_t : bool { run_many, run_once };

					// A single callback and info associated with it
				private:
					boost::any m_callback;
					callback_id_t m_id;
					size_t m_arity;
					run_mode_t m_run_mode;

				public:
					template<typename CallbackItem>
					callback_info_t( CallbackItem callback_item, size_t arity, run_mode_t run_mode = run_mode_t::run_many )
					  : m_callback{std::move( callback_item )}, m_id{get_next_id( )}, m_arity{arity}, m_run_mode{run_mode} {

						daw::exception::daw_throw_on_true( m_callback.empty( ), "Callback should never be empty" );
					}

					callback_info_t( ) = delete;
					~callback_info_t( ) = default;
					callback_info_t( callback_info_t const & ) = default;
					callback_info_t( callback_info_t && ) noexcept = default;
					callback_info_t &operator=( callback_info_t const & ) = default;
					callback_info_t &operator=( callback_info_t && ) noexcept = default;

					callback_id_t id( ) const noexcept {
						return m_id;
					}

					template<typename ReturnType = void, typename... Args>
					void operator( )( Args &&... args ) const {
						using cb_type =
						  std::function<daw::traits::root_type_t<ReturnType>( typename daw::traits::root_type_t<Args>... )>;
						auto const callback = boost::any_cast<cb_type>( m_callback );
						callback( std::forward<Args>( args )... );
					}

					explicit operator bool( ) const noexcept {
						return !m_callback.empty( );
					}

					bool remove_after_run( ) const noexcept {
						return m_run_mode == run_mode_t::run_once;
					}

					size_t arity( ) const noexcept {
						return m_arity;
					}

				private:
					callback_id_t get_next_id( ) const noexcept {
						static std::atomic<callback_id_t> s_last_id{1};
						return s_last_id++;
					}
				};
				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Allows for the dispatch of events to subscribed listeners
				///				Callbacks can be be c-style function pointers, lambda's or
				///				std::function with the correct signature.
				///	Requires:	base::Callback
				template<size_t MaxEventCount>
				struct EventEmitterImpl {
					using listeners_t = daw::fixed_lookup<std::vector<callback_info_t>, MaxEventCount>;
					using callback_id_t = typename callback_info_t::callback_id_t;
					using callback_run_mode_t = callback_info_t::run_mode_t;

				private:
					static constexpr int_least8_t const c_max_emit_depth = 100; // TODO: Magic Number

					listeners_t m_listeners;
					size_t m_max_listeners;
					std::shared_ptr<std::atomic_int_least8_t> m_emit_depth;

				public:
					explicit EventEmitterImpl( size_t max_listeners )
					  : m_listeners{}
					  , m_max_listeners{max_listeners}
					  , m_emit_depth{std::make_shared<std::atomic_int_least8_t>( 0 )} {}

					EventEmitterImpl( EventEmitterImpl const & ) = delete;
					EventEmitterImpl &operator=( EventEmitterImpl const & ) = delete;

					EventEmitterImpl( EventEmitterImpl && ) noexcept = default;
					EventEmitterImpl &operator=( EventEmitterImpl && ) noexcept = default;

					virtual ~EventEmitterImpl( ) = default;

					friend bool operator==( EventEmitterImpl const &lhs, EventEmitterImpl const &rhs ) noexcept {
						return &lhs == &rhs; // All we need is a pointer comparison
					}

					friend bool operator!=( EventEmitterImpl const &lhs, EventEmitterImpl const &rhs ) noexcept {
						return &lhs != &rhs; // All we need is a pointer comparison
					}

					friend bool operator<( EventEmitterImpl const &lhs, EventEmitterImpl const &rhs ) noexcept {
						return std::less<EventEmitterImpl const *const>{}( &lhs, &rhs );
					}

					void remove_all_callbacks( daw::string_view event ) {
						m_listeners[event].clear( );
					}

					size_t &max_listeners( ) noexcept {
						return m_max_listeners;
					}

					size_t const &max_listeners( ) const noexcept {
						return m_max_listeners;
					}

				protected:
					listeners_t &listeners( ) noexcept {
						return m_listeners;
					}

					listeners_t const &listeners( ) const noexcept {
						return m_listeners;
					}

					std::vector<callback_info_t> &get_callbacks_for( daw::string_view cb_name ) {
						return listeners( )[cb_name];
					}

					std::vector<callback_info_t> const &get_callbacks_for( daw::string_view cb_name ) const {
						return listeners( )[cb_name];
					}

				public:
					size_t listener_count( daw::string_view event_name ) const {
						return listeners( )[event_name].size( );
					}

					template<typename... ExpectedArgs, typename Listener>
					callback_id_t add_listener( daw::string_view event, Listener listener,
					                            callback_run_mode_t run_mode = callback_run_mode_t::run_many ) {
						daw::exception::daw_throw_on_true( event.empty( ), "Empty event name passed to add_listener" );
						daw::exception::daw_throw_on_true( at_max_listeners( event ), "Max listeners reached for event" );

						constexpr auto const callback_obj = impl::make_listener_t<Listener, ExpectedArgs...>( );
						callback_info_t callback{callback_obj.create( std::move( listener ) ), callback_obj.arity, run_mode};
						auto callback_id = callback.id( );
						if( event != "newListener" ) {
							emit_listener_added( event, callback_id );
						}
						get_callbacks_for( event ).push_back( std::move( callback ) );
						return callback_id;
					}

				private:
					template<typename... Args>
					void emit_impl( daw::string_view event, Args &&... args ) {
						std::vector<callback_info_t> &callbacks = get_callbacks_for( event );
						for( callback_info_t const &callback : callbacks ) {
							if( callback.arity( ) == 0 ) {
								callback( );
							} else if( sizeof...( Args ) == callback.arity( ) ) {
								callback( std::forward<Args>( args )... );
							} else {
								daw::exception::daw_throw( "Number of expected arguments does not match that provided" );
							}
						}
						daw::algorithm::erase_remove_if( callbacks,
						                                 []( callback_info_t const &item ) { return item.remove_after_run( ); } );
						--( *m_emit_depth );
					}

				public:
					template<typename... Args>
					void emit( daw::string_view event, Args &&... args ) {
						daw::exception::daw_throw_on_true( event.empty( ), "Empty event name passed to emit" );
						++( *m_emit_depth );
						daw::exception::daw_throw_on_true( *m_emit_depth > c_max_emit_depth,
						                                   "Max callback depth reached.  Possible loop" );

						emit_impl( event, std::forward<Args>( args )... );
						auto const event_selfdestruct = event.to_string( ) + "_selfdestruct";
						if( m_listeners.exists( event_selfdestruct ) ) {
							emit_impl( event_selfdestruct ); // Called by self destruct code and must be last so
							                                 // lifetime is controlled
						}
					}

					void emit_listener_added( daw::string_view event, callback_id_t callback_id ) {
						emit( "listener_added", event.to_string( ), callback_id );
					}

					void emit_listener_removed( daw::string_view event, callback_id_t callback_id ) {
						emit( "listener_removed", event.to_string( ), callback_id );
					}

					bool at_max_listeners( daw::string_view event ) {
						auto result = 0 != m_max_listeners; // Zero means no limit
						result &= get_callbacks_for( event ).size( ) >= m_max_listeners;
						return result;
					}

					static EventEmitter create( size_t max_listeners = 10 ) noexcept {
						try {
							auto result = new EventEmitterImpl{max_listeners};
							return EventEmitter{result};
						} catch( ... ) { return EventEmitter{nullptr}; }
					}
				}; // class EventEmitterImpl
			}    // namespace impl

			template<size_t MaxEventCount = impl::DefaultMaxEventCount>
			EventEmitter create_event_emitter( size_t max_listeners = 10 ) noexcept {
				return impl::EventEmitterImpl<MaxEventCount>::create( max_listeners );
			}

			//////////////////////////////////////////////////////////////////////////
			// Allows one to have the Events defined in event emitter
			template<typename Derived>
			struct StandardEvents {
				using callback_id_t = impl::callback_info_t::callback_id_t;
				using callback_runmode_t = impl::callback_info_t::run_mode_t;

			private:
				daw::nodepp::base::EventEmitter m_emitter;

				Derived &child( ) {
					return *static_cast<Derived *>( this );
				}

				void emit_error( base::Error error ) {
					m_emitter->emit( "error", std::move( error ) );
				}

				template<typename DestinationType>
				void detect_delegate_loops( std::weak_ptr<DestinationType> destination_obj ) const {
					if( destination_obj.expired( ) ) {
						return;
					}
					auto destination_obj_sp = destination_obj.lock( );
					daw::exception::daw_throw_on_false( destination_obj_sp,
					                                    "Destination event handler is out of scope while adding delegate" );
					if( *destination_obj_sp->emitter( ) == *this->emitter( ) ) {
						daw::exception::daw_throw( "Attempt to delegate to self.  This is a callback loop" );
					}
				}

			public:
				StandardEvents( ) = delete;
				explicit StandardEvents( daw::nodepp::base::EventEmitter emitter ) : m_emitter( std::move( emitter ) ) {}

				virtual ~StandardEvents( ) = default;
				StandardEvents( StandardEvents const & ) = default;
				StandardEvents( StandardEvents && ) noexcept = default;
				StandardEvents &operator=( StandardEvents const & ) = default;
				StandardEvents &operator=( StandardEvents && ) noexcept = default;

				EventEmitter &emitter( ) {
					return m_emitter; // If you get a warning about a recursive call here, you forgot to create a
					                  // emitter() in Derived
				}

				EventEmitter const &emitter( ) const {
					return m_emitter; // If you get a warning about a recursive call here, you forgot to create a
					                  // emitter() in Derived
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Callback is for when error's occur
				template<typename Listener>
				Derived &on_error( Listener listener ) {
					m_emitter->template add_listener<base::Error>( "error", std::move( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Callback is for the next error
				Derived &on_next_error( std::function<void( base::Error )> listener ) {
					m_emitter->template add_listener<base::Error>( "error", std::move( listener ), callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Callback is called whenever a new listener is added for
				///				any callback
				template<typename Listener>
				Derived &on_listener_added( Listener listener ) {
					m_emitter->template add_listener<std::string, callback_id_t>( "listener_added", std::move( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Callback is called when the next new listener is added
				///				for any callback
				template<typename Listener>
				Derived &on_next_listener_added( Listener listener ) {
					m_emitter->template add_listener<std::string, callback_id_t>( "listener_added", std::move( listener ),
					                                                              callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Callback is called whenever a listener is removed for
				/// any callback
				template<typename Listener>
				Derived &on_listener_removed( Listener listener ) {
					m_emitter->template add_listener<std::string, callback_id_t>( "listener_removed", std::move( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Callback is called the next time a listener is removed for
				/// any callback
				template<typename Listener>
				Derived &on_next_listener_removed( Listener listener ) {
					m_emitter->template add_listener<std::string, callback_id_t>( "listener_removed", std::move( listener ),
					                                                              callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Callback is called when the subscribed object is exiting.
				///				This does not necessarily, but can be, from it's
				///				destructor.  Make sure to wrap in try/catch if in
				///				destructor
				template<typename Listener>
				Derived &on_exit( Listener listener ) {
					m_emitter->template add_listener<OptionalError>( "exit", std::move( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Callback is called when the subscribed object is exiting.
				///				This does not necessarily, but can be, from it's
				///				destructor.  Make sure to wrap in try/catch if in
				///				destructor
				template<typename Listener>
				Derived &on_next_exit( Listener listener ) {
					m_emitter->template add_listener<OptionalError>( "exit", std::move( listener ),
					                                                 callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Delegate error callbacks to another error handler
				///
				/// @param error_destination A weak_ptr to destination object
				/// @param description Possible description of error
				/// @param where Where on_error was called from
				template<typename StandardEventsChild>
				Derived &on_error( std::weak_ptr<StandardEventsChild> error_destination, std::string description,
				                   std::string where ) {
					on_error( [error_destination, description, where]( base::Error const &error ) {
						if( !error_destination.expired( ) ) {
							auto obj = error_destination.lock( );
							if( obj ) {
								obj->emit_error( error, description, where );
							}
						}
					} );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Delegate error callbacks to another error handler
				///
				/// @param error_destination A shared_ptr to destination object
				/// @param description Possible description of error
				/// @param where Where on_error was called from
				template<typename StandardEventsChild>
				Derived &on_error( std::shared_ptr<StandardEventsChild> error_destination, std::string description,
				                   std::string where ) {
					return on_error( std::weak_ptr<StandardEventsChild>( error_destination ), std::move( description ),
					                 std::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Emit an error event
				void emit_error( daw::string_view description, daw::string_view where ) {
					base::Error err{description};
					err.add( "where", where );

					emit_error( std::move( err ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Emit an error event
				void emit_error( base::Error const &child, daw::string_view description, daw::string_view where ) {
					base::Error err{description};
					err.add( "derived_error", "true" );
					err.add( "where", where );
					err.add_child( child );

					emit_error( std::move( err ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Emit an error event
				void emit_error( ErrorCode const &error, daw::string_view description, daw::string_view where ) {
					base::Error err{description, error};
					err.add( "where", where.to_string( ) );

					emit_error( std::move( err ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary: Emit an error event
				void emit_error( std::exception_ptr ex, daw::string_view description, daw::string_view where ) {
					base::Error err{description, std::move( ex )};
					err.add( "where", where );

					emit_error( std::move( err ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Emit an event with the callback and event name of a newly
				///				added event
				void emit_listener_added( daw::string_view event, callback_id_t callback_id ) {
					emitter( )->emit_listener_added( event, callback_id );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Emit an event with the callback and event name of an event
				///				that has been removed
				void emit_listener_removed( daw::string_view event, callback_id_t callback_id ) {
					emitter( )->emit( "listener_removed", event, callback_id );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Emit and event when exiting to alert others that they
				///				may want to stop and exit. This version allows for an
				///				error reason
				void emit_exit( Error error ) {
					m_emitter->emit( "exit", create_optional_error( std::move( error ) ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Emit and event when exiting to alert others that they
				///				may want to stop and exit.
				void emit_exit( ) {
					m_emitter->emit( "exit", create_optional_error( ) );
				}

				template<typename Class, typename Func,
				         typename ResultType = std::decay_t<decltype( std::declval<Func>( )( ) )>,
				         typename = std::enable_if_t<!is_same_v<void, ResultType>>>
				static boost::optional<ResultType> emit_error_on_throw( std::shared_ptr<Class> self,
				                                                        daw::string_view err_description,
				                                                        daw::string_view where, Func func ) {
					try {
						return func( );
					} catch( ... ) {
						self->emit_error( std::current_exception( ), err_description, where );
						return boost::none;
					}
				}

				template<typename Class, typename Func,
				         typename ResultType = std::decay_t<decltype( std::declval<Func>( )( ) )>,
				         typename = std::enable_if_t<is_same_v<void, ResultType>>>
				static void emit_error_on_throw( std::shared_ptr<Class> self, daw::string_view err_description,
				                                 daw::string_view where, Func func ) {
					try {
						func( );
					} catch( ErrorCode const &err ) { self->emit_error( err, err_description, where ); } catch( ... ) {
						self->emit_error( std::current_exception( ), err_description, where );
					}
				}

				template<typename Class, typename Func>
				static void run_if_valid( std::weak_ptr<Class> obj, daw::string_view err_description, daw::string_view where,
				                          Func func ) {
					if( !obj.expired( ) ) {
						auto self = obj.lock( );
						emit_error_on_throw(
						  self, err_description, where,
						  [ func = std::move( func ), self = std::move( self ) ]( ) mutable { func( std::move( self ) ); } );
					}
				}

				//////////////////////////////////////////////////////////////////////////
				/// Summary:	Creates a callback on the event source that calls a
				///				mirroring emitter on the destination obj. Unless the
				///				callbacks are of the form std::function<void( )> the
				///				callback parameters must be template parameters here.
				///				e.g.
				///				obj_emitter.delegate_to<daw::nodepp::lib::net::EndPoint>( "listening",
				/// dest_obj.get_weak_ptr(
				///), "listening" );
				template<typename... Args, typename DestinationType>
				Derived &delegate_to( daw::string_view source_event, std::weak_ptr<DestinationType> destination_obj,
				                      std::string destination_event ) {
					if( !destination_obj.expired( ) ) {
						detect_delegate_loops( destination_obj );
						auto handler = [destination_obj, destination_event]( Args... args ) {
							if( !destination_obj.expired( ) ) {
								auto obj = destination_obj.lock( );
								if( obj ) {
									obj->emitter( )->emit( destination_event, args... );
								}
							}
						};
						m_emitter->template add_listener<Args...>( source_event, handler );
					}
					return child( );
				}
			}; // class StandardEvents
		}    // namespace base
	}      // namespace nodepp
} // namespace daw
