// The MIT License (MIT)
// Copyright (c) 2014-2018 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
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

#include <daw/daw_container_algorithm.h>
#include <daw/daw_fixed_lookup.h>
#include <daw/daw_string_fmt.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>
#include <daw/parallel/daw_observable_ptr_pair.h>

#include "base_error.h"
#include "base_memory.h"

namespace daw {
	namespace nodepp {
		namespace base {
			namespace impl {
				constexpr size_t DefaultMaxEventCount = 20;

				template<typename Listener, typename... ExpectedArgs>
				constexpr bool is_valid_listener_v =
				  daw::is_callable_v<Listener, ExpectedArgs...>;

				template<typename Listener, typename... ExpectedArgs,
				         std::enable_if_t<daw::is_callable_v<Listener, ExpectedArgs...>,
				                          std::nullptr_t> = nullptr>
				constexpr auto make_listener_t( ) noexcept {
					struct result_t {
						size_t const arity = sizeof...( ExpectedArgs );

						std::function<void( ExpectedArgs... )>
						create( Listener &&listener ) const {
							return [listener = std::forward<Listener>( listener )](
							         ExpectedArgs... args ) mutable {
								listener( std::move( args )... );
							};
						}
					};
					return result_t( );
				};

				template<typename Listener, typename... ExpectedArgs,
				         std::enable_if_t<sizeof...( ExpectedArgs ) != 0 &&
				                            daw::is_callable_v<Listener>,
				                          std::nullptr_t> = nullptr>
				constexpr auto make_listener_t( ) noexcept {
					struct result_t {
						size_t const arity = 0;

						std::function<void( )> create( Listener &&listener ) const {
							return
							  [listener = std::forward<Listener>( listener )]( ) mutable {
								  listener( );
							  };
						}
						constexpr void create( std::nullptr_t ) const = delete;
					};
					return result_t( );
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
					callback_info_t( CallbackItem &&callback_item, size_t arity,
					                 run_mode_t run_mode = run_mode_t::run_many )
					  : m_callback( std::forward<CallbackItem>( callback_item ) )
					  , m_id( get_next_id( ) )
					  , m_arity( arity )
					  , m_run_mode( run_mode ) {

						daw::exception::daw_throw_on_true(
						  m_callback.empty( ), "Callback should never be empty" );
					}

					callback_id_t id( ) const noexcept {
						return m_id;
					}

					template<typename ReturnType = void, typename... Args>
					void operator( )( Args &&... args ) const {
						using cb_type = std::function<daw::traits::root_type_t<ReturnType>(
						  typename daw::traits::root_type_t<Args>... )>;
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
				/// @brief	Allows for the dispatch of events to subscribed listeners
				///				Callbacks can be be c-style function pointers, lambda's or
				///				a callable with the correct signature.
				///	Requires:	base::Callback
				template<size_t MaxEventCount>
				struct basic_event_emitter {
					using listeners_t =
					  daw::fixed_lookup<std::vector<callback_info_t>, MaxEventCount, 4>;
					using callback_id_t = typename callback_info_t::callback_id_t;
					using callback_run_mode_t = callback_info_t::run_mode_t;

				private:
					static constexpr int_least8_t const c_max_emit_depth =
					  100; // TODO: Magic Number

					listeners_t m_listeners;
					size_t m_max_listeners;
					int_least8_t m_emit_depth;

					listeners_t &listeners( ) noexcept {
						return m_listeners;
					}

					listeners_t const &listeners( ) const noexcept {
						return m_listeners;
					}

					std::vector<callback_info_t> &
					get_callbacks_for( daw::string_view cb_name ) {
						return listeners( )[cb_name];
					}

					template<typename... Args>
					void emit_impl( daw::string_view event, Args &&... args ) {
						auto callbacks = get_callbacks_for( event );
						for( callback_info_t const &callback : callbacks ) {
							if( callback.arity( ) == 0 ) {
								callback( );
							} else if( sizeof...( Args ) == callback.arity( ) ) {
								callback( std::forward<Args>( args )... );
							} else {
								daw::exception::daw_throw(
								  "Number of expected arguments does not match that provided" );
							}
						}
						daw::container::erase_remove_if( callbacks,
						                                 []( callback_info_t const &item ) {
							                                 return item.remove_after_run( );
						                                 } );

						--m_emit_depth;
					}

				public:
					explicit basic_event_emitter( size_t max_listeners )
					  : m_listeners( )
					  , m_max_listeners( max_listeners )
					  , m_emit_depth( 0 ) {}

					basic_event_emitter( basic_event_emitter const & ) = delete;
					basic_event_emitter &
					operator=( basic_event_emitter const & ) = delete;

					bool is_same_instance( basic_event_emitter const &rhs ) const
					  noexcept {
						return this == &rhs;
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

					size_t listener_count( daw::string_view event_name ) const {
						return listeners( )[event_name].size( );
					}

					template<typename... ExpectedArgs, typename Listener>
					callback_id_t add_listener(
					  daw::string_view event, Listener &&listener,
					  callback_run_mode_t run_mode = callback_run_mode_t::run_many ) {

						static_assert( is_callable_v<Listener, ExpectedArgs...> ||
						                 is_callable_v<Listener>,
						               "Listener does not accept expected arguments" );

						daw::exception::daw_throw_on_true(
						  event.empty( ), "Empty event name passed to add_listener" );

						daw::exception::daw_throw_on_true(
						  at_max_listeners( event ), "Max listeners reached for event" );

						constexpr auto const callback_obj =
						  impl::make_listener_t<Listener, ExpectedArgs...>( );

						auto callback = callback_info_t(
						  callback_obj.create( std::forward<Listener>( listener ) ),
						  callback_obj.arity, run_mode );

						auto callback_id = callback.id( );
						if( event != "newListener" ) {
							emit_listener_added( event, callback_id );
						}
						get_callbacks_for( event ).push_back( std::move( callback ) );
						return callback_id;
					}

					template<typename... Args>
					void emit( daw::string_view event, Args &&... args ) {
						daw::exception::daw_throw_on_true(
						  event.empty( ), "Empty event name passed to emit" );
						{
							auto const depth = ++m_emit_depth;
							daw::exception::daw_throw_on_true(
							  depth > c_max_emit_depth,
							  "Max callback depth reached.  Possible loop" );
						}
						try {
							emit_impl( event, std::forward<Args>( args )... );
							auto const event_selfdestruct =
							  daw::fmt( "{0}_selfdestruct", event );
							if( m_listeners.exists( event_selfdestruct ) ) {
								emit_impl(
								  event_selfdestruct ); // Called by self destruct code and must
								                        // be last so lifetime is controlled
							}
						} catch( ... ) {
							--m_emit_depth;
							throw;
						}
						--m_emit_depth;
					}

					void emit_listener_added( daw::string_view event,
					                          callback_id_t callback_id ) {
						emit( "listener_added", event.to_string( ), callback_id );
					}

					void emit_listener_removed( daw::string_view event,
					                            callback_id_t callback_id ) {
						emit( "listener_removed", event.to_string( ), callback_id );
					}

					bool at_max_listeners( daw::string_view event ) {
						auto result = 0 != m_max_listeners; // Zero means no limit
						result &= get_callbacks_for( event ).size( ) >= m_max_listeners;
						return result;
					}
				}; // class basic_event_emitter
			}    // namespace impl

			class EventEmitter {
				using emitter_t = impl::basic_event_emitter<impl::DefaultMaxEventCount>;
				daw::observable_ptr_pair<emitter_t> m_emitter;

			public:
				using callback_id_t = impl::callback_info_t::callback_id_t;
				using callback_run_mode_t = impl::callback_info_t::run_mode_t;

				explicit EventEmitter( size_t max_listeners = 10 );

				void remove_all_callbacks( daw::string_view event );
				size_t &max_listeners( ) noexcept;
				size_t const &max_listeners( ) const noexcept;
				size_t listener_count( daw::string_view event_name ) const;

				template<typename... ExpectedArgs, typename Listener>
				callback_id_t add_listener(
				  daw::string_view event, Listener &&listener,
				  callback_run_mode_t run_mode = callback_run_mode_t::run_many ) {

					return m_emitter.visit( [&]( auto &em ) {
						return em.template add_listener<ExpectedArgs...>(
						  event, std::forward<Listener>( listener ), run_mode );
					} );
				}

				template<typename... Args>
				void emit( daw::string_view event, Args &&... args ) {
					m_emitter.visit( [&]( auto &em ) {
						return em.emit( event, std::forward<Args>( args )... );
					} );
				}

				bool is_same_instance( EventEmitter const &em ) const;

				void emit_listener_added( daw::string_view event,
				                          callback_id_t callback_id );
				void emit_listener_removed( daw::string_view event,
				                            callback_id_t callback_id );
				bool at_max_listeners( daw::string_view event );

				void emit_error( base::Error error );

				/// @brief Emit an error event
				/// @param description Error desciption text
				/// @param where identity of method that error occurred
				void emit_error( std::string description, std::string where );

				/// @brief Emit an error event
				/// @param child A child error event to get a stack trace of errors
				/// @param description Description text
				/// @param where where in code error happened
				void emit_error( base::Error const &child, std::string description,
				                 std::string where );

				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( ErrorCode const &error, std::string description,
				                 std::string where );

				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( std::exception_ptr ex, std::string description,
				                 std::string where );
			};

			//////////////////////////////////////////////////////////////////////////
			// Allows one to have the Events defined in event emitter
			template<typename Derived>
			struct StandardEvents {
				using callback_id_t = impl::callback_info_t::callback_id_t;
				using callback_runmode_t = impl::callback_info_t::run_mode_t;

			private:
				daw::nodepp::base::EventEmitter m_emitter;

				inline Derived &child( ) noexcept {
					return *static_cast<Derived *>( this );
				}

				inline Derived const &child( ) const noexcept {
					return *static_cast<Derived const *>( this );
				}

				void emit_error( base::Error error ) {
					m_emitter.emit_error( std::move( error ) );
				}

				void detect_delegate_loops( EventEmitter const &em ) const {
					if( emitter( ).is_same_instance( em ) ) {
						daw::exception::daw_throw(
						  "Attempt to delegate to self.  This is a callback loop" );
					}
				}

			public:
				explicit StandardEvents( daw::nodepp::base::EventEmitter &&emitter )
						: m_emitter( std::move( emitter ) ) {}

				explicit StandardEvents( daw::nodepp::base::EventEmitter const &emitter )
				  : m_emitter( emitter ) {}

				EventEmitter &emitter( ) {
					return m_emitter; // If you get a warning about a recursive call here,
					                  // you forgot to create a emitter() in Derived
				}

				EventEmitter const &emitter( ) const {
					return m_emitter; // If you get a warning about a recursive call here,
					                  // you forgot to create a emitter() in Derived
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is for when error's occur
				template<typename Listener>
				Derived &on_error( Listener &&listener ) {
					m_emitter.template add_listener<base::Error>(
					  "error", std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is for the next error
				template<typename Listener>
				Derived &on_next_error( Listener &&listener ) {
					m_emitter.template add_listener<base::Error>(
					  "error", std::forward<Listener>( listener ),
					  callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called whenever a new listener is added for
				///				any callback
				template<typename Listener>
				Derived &on_listener_added( Listener &&listener ) {
					m_emitter.template add_listener<std::string, callback_id_t>(
					  "listener_added", std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called when the next new listener is added
				///				for any callback
				template<typename Listener>
				Derived &on_next_listener_added( Listener &&listener ) {
					m_emitter.template add_listener<std::string, callback_id_t>(
					  "listener_added", std::forward<Listener>( listener ),
					  callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is called whenever a listener is removed for
				/// any callback
				template<typename Listener>
				Derived &on_listener_removed( Listener &&listener ) {
					m_emitter.template add_listener<std::string, callback_id_t>(
					  "listener_removed", std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is called the next time a listener is removed for
				/// any callback
				template<typename Listener>
				Derived &on_next_listener_removed( Listener &&listener ) {
					m_emitter.template add_listener<std::string, callback_id_t>(
					  "listener_removed", std::forward<Listener>( listener ),
					  callback_runmode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called when the subscribed object is exiting.
				///				This does not necessarily, but can be, from it's
				///				destructor.  Make sure to wrap in try/catch if in
				///				destructor
				template<typename Listener>
				Derived &on_exit( Listener &&listener ) {
					m_emitter.template add_listener<OptionalError>(
					  "exit", std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called when the subscribed object is exiting.
				///				This does not necessarily, but can be, from it's
				///				destructor.  Make sure to wrap in try/catch if in
				///				destructor
				template<typename Listener>
				Derived &on_next_exit( Listener &&listener ) {
					m_emitter.template add_listener<OptionalError>(
					  "exit", std::forward<Listener>( listener ),
					  callback_runmode_t::run_once );
					return child( );
				}

				/// @brief Delegate error callbacks to another error handler
				/// @param error_destination A weak_ptr to destination object
				/// @param description Possible description of error
				/// @param where Where on_error was called from
				Derived &on_error( EventEmitter error_destination,
				                   std::string description, std::string where ) {
					on_error(
					  [error_destination = std::move( error_destination ),
					   description = std::move( description ),
					   where = std::move( where )]( base::Error const &error ) mutable {
						  error_destination.emit_error( error, description, where );
					  } );
					return child( );
				}

				/*
				//////////////////////////////////////////////////////////////////////////
				/// @brief Delegate error callbacks to another error handler
				/// @param error_destination A shared_ptr to destination object with an
				///	emitter
				/// @param description Possible description of error
				/// @param where Where on_error was called from

				template<typename StandardEventsChild>
				Derived &on_error( daw::observable_ptr<StandardEventsChild>
				error_destination, std::string description, std::string where ) { return
				on_error( std::weak_ptr<StandardEventsChild>( error_destination ),
				std::move( description ), std::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Delegate error callbacks to another error handler
				/// @param error_destination Object with emitter to send events too
				/// @param description Possible description of error
				/// @param where Where on_error was called from
				template<typename StandardEventsChild>
				Derived &on_error( StandardEvents<StandardEventsChild>
				&error_destination, std::string description, std::string where ) {
				  on_error( [ obj = error_destination.obs_emiter( ), description, where
				]( base::Error const &error ) mutable { obj.lock( [&]( auto &self ) {
				self.emit( "error", error, description, where ); } ); } ); return child(
				);
				}
				*/
				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( std::string description, std::string where ) {
					m_emitter.emit_error( std::move( description ), std::move( where ) );
				}

				/// @brief Emit an error event
				/// @param child A child error event to get a stack trace of errors
				/// @param description Description text
				/// @param where where in code error happened
				void emit_error( base::Error const &child, std::string description,
				                 std::string where ) {
					m_emitter.emit_error( child, std::move( description ),
					                      std::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( ErrorCode const &error, std::string description,
				                 std::string where ) {
					m_emitter.emit_error( error, std::move( description ),
					                      std::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( std::exception_ptr ex, std::string description,
				                 std::string where ) {
					m_emitter.emit_error( ex, std::move( description ),
					                      std::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit an event with the callback and event name of a newly
				///				added event
				void emit_listener_added( std::string event,
				                          callback_id_t callback_id ) {
					emitter( ).emit_listener_added( std::move( event ), callback_id );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit an event with the callback and event name of an event
				///				that has been removed
				void emit_listener_removed( std::string event,
				                            callback_id_t callback_id ) {
					emitter( ).emit( "listener_removed", std::move( event ),
					                 callback_id );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit and event when exiting to alert others that they
				///				may want to stop and exit. This version allows for an
				///				error reason
				void emit_exit( Error error ) {
					m_emitter.emit( "exit", create_optional_error( std::move( error ) ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit and event when exiting to alert others that they
				///				may want to stop and exit.
				void emit_exit( ) {
					m_emitter.emit( "exit", create_optional_error( ) );
				}

				template<typename Func,
				         typename ResultType =
				           std::decay_t<decltype( std::declval<Func>( )( ) )>,
				         typename = std::enable_if_t<!is_same_v<void, ResultType>>>
				static boost::optional<ResultType>
				emit_error_on_throw( EventEmitter &em, daw::string_view err_description,
				                     daw::string_view where, Func &&func ) {
					static_assert( daw::is_callable_v<Func>,
					               "Func must be callable without arguments" );
					try {
						return func( );
					} catch( ... ) {
						em.emit_error( std::current_exception( ), err_description, where );
						return boost::none;
					}
				}

				template<typename Class, typename Func>
				static void run_if_valid( std::weak_ptr<Class> obj,
				                          daw::string_view err_description,
				                          daw::string_view where, Func &&func ) {
					static_assert( daw::is_callable_v<Func, decltype( obj.lock( ) )>,
					               "Func must be callable with shared_ptr<Class>" );

					if( !obj.expired( ) ) {
						auto self = obj.lock( );
						emit_error_on_throw(
						  self, err_description, where,
						  [func = std::forward<Func>( func ), self]( ) mutable {
							  func( std::move( self ) );
						  } );
					}
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Creates a callback on the event source that calls a
				///				mirroring emitter on the destination obj. Unless the
				///				callbacks are of the form void( ) the
				///				callback parameters must be template parameters here.
				template<typename... Args>
				Derived &delegate_to( daw::string_view source_event, EventEmitter &&em,
				                      std::string destination_event ) {
					detect_delegate_loops( em );
					m_emitter.template add_listener<Args...>(
					  source_event,
					  [em = std::move( em ), destination_event]( Args... args ) mutable {
						  em.emit( destination_event, args... );
					  } );
					return child( );
				}

				template<typename... Args>
				Derived &delegate_to( daw::string_view source_event, EventEmitter &em,
				                      std::string destination_event ) {
					detect_delegate_loops( em );
					m_emitter.template add_listener<Args...>(
					  source_event, [em, destination_event]( Args... args ) mutable {
						  em.emit( destination_event, args... );
					  } );
					return child( );
				}
			}; // class StandardEvents
		}    // namespace base
	}      // namespace nodepp
} // namespace daw
