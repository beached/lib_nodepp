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

#include <any>
#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <daw/daw_container_algorithm.h>
#include <daw/daw_fixed_lookup.h>
#include <daw/daw_stack_function.h>
#include <daw/daw_string_fmt.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>

#include "base_error.h"

namespace daw {
	namespace nodepp {
		namespace base {
			template<typename... Args>
			using cb_storage_type = std::function<Args...>;

			enum class callback_run_mode_t : bool { run_many, run_once };
			namespace ee_impl {
				constexpr size_t DefaultMaxEventCount = 20;

				template<typename Listener, typename... ExpectedArgs>
				inline constexpr bool is_valid_listener_v =
				  std::is_invocable_v<Listener, ExpectedArgs...>;

				template<
				  typename Listener, typename... ExpectedArgs,
				  std::enable_if_t<std::is_invocable_v<Listener, ExpectedArgs...>,
				                   std::nullptr_t> = nullptr>
				auto make_listener_t( ) {
					struct result_t {
						size_t const arity = sizeof...( ExpectedArgs );

						static cb_storage_type<void( ExpectedArgs... )>
						create( Listener &&listener ) {
							auto result = cb_storage_type<void( ExpectedArgs... )>(
							  std::forward<Listener>( listener ) );
							daw::exception::precondition_check(
							  result, "Expected callable listener" );
							return result;
						}
						static void create( std::nullptr_t ) = delete;
					};
					return result_t{};
				};

				template<typename Listener, typename... ExpectedArgs,
				         std::enable_if_t<sizeof...( ExpectedArgs ) != 0 and
				                            std::is_invocable_v<Listener>,
				                          std::nullptr_t> = nullptr>
				auto make_listener_t( ) {
					struct result_t {
						size_t const arity = sizeof...( ExpectedArgs );

						static cb_storage_type<void( ExpectedArgs &&... )>
						create( Listener &&listener ) {
							return [listener =
							          mutable_capture( std::forward<Listener>( listener ) )](
							         ExpectedArgs &&... ) { daw::invoke( *listener ); };
						}
						static void create( std::nullptr_t ) = delete;
					};
					return result_t{};
				}

				struct callback_info_t {
					using callback_id_t = size_t;

					// A single callback and info associated with it
				private:
					// Using any because we do know know what type the function being
					// stored
					std::any m_callback;
					callback_id_t m_id = get_next_id( );
					// A check if the type is argumentless or not but also a sanity check
					// on the arguments
					size_t m_arity;
					// How many times to run, once or many
					callback_run_mode_t m_run_mode = callback_run_mode_t::run_many;

				public:
					template<typename CallbackItem>
					callback_info_t( CallbackItem &&callback_item, size_t arity )
					  : m_callback( std::forward<CallbackItem>( callback_item ) )
					  , m_arity( arity ) {

						daw::exception::precondition_check(
						  m_callback.has_value( ), "Callback should never be empty" );
					}

					template<typename CallbackItem>
					callback_info_t( CallbackItem &&callback_item, size_t arity,
					                 callback_run_mode_t run_mode )
					  : m_callback( std::forward<CallbackItem>( callback_item ) )
					  , m_arity( arity )
					  , m_run_mode( run_mode ) {

						daw::exception::precondition_check(
						  m_callback.has_value( ), "Callback should never be empty" );
					}

					callback_id_t id( ) const noexcept {
						return m_id;
					}

					template<typename ReturnType = void, typename... Args>
					void operator( )( Args &&... args ) const {
						using cb_type =
						  cb_storage_type<daw::traits::root_type_t<ReturnType>(
						    typename daw::traits::root_type_t<Args>... )>;

						auto const callback = std::any_cast<cb_type>( m_callback );
						daw::invoke( callback, std::forward<Args>( args )... );
					}

					template<typename ReturnType = void, typename... Args>
					void operator( )( Args &&... args ) {
						using cb_type =
						  cb_storage_type<daw::traits::root_type_t<ReturnType>(
						    typename daw::traits::root_type_t<Args>... )>;

						auto const callback = std::any_cast<cb_type>( m_callback );
						daw::invoke( callback, std::forward<Args>( args )... );
					}

					explicit operator bool( ) const noexcept {
						return m_callback.has_value( );
					}

					bool remove_after_run( ) const noexcept {
						return m_run_mode == callback_run_mode_t::run_once;
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
					//					using listeners_t =
					//					  daw::fixed_lookup<std::vector<callback_info_t>,
					// MaxEventCount, 4>;
					using listeners_t =
					  std::unordered_map<std::string, std::vector<callback_info_t>>;
					using callback_id_t = typename callback_info_t::callback_id_t;

				private:
					static constexpr int_least8_t const c_max_emit_depth =
					  100; // TODO: Magic Number

					listeners_t m_listeners{};
					size_t m_max_listeners;
					std::atomic_int_least8_t m_emit_depth = 0;

					listeners_t &listeners( ) noexcept {
						return m_listeners;
					}

					listeners_t const &listeners( ) const noexcept {
						return m_listeners;
					}

					std::vector<callback_info_t> &
					get_callbacks_for( daw::string_view cb_name ) {
						return m_listeners[cb_name];
					}

					template<typename... Args>
					void emit_impl( daw::string_view event, Args &&... args ) {
						daw::container::erase_remove_if(
						  get_callbacks_for( event ), [&]( auto &&callback ) {
							  daw::exception::precondition_check(
							    sizeof...( Args ) == callback.arity( ),
							    "Number of expected arguments does not match that provided" );

							  std::cout << "Event: " << event << '\n';
							  // Cannot forward arguments as more than one callback could be
							  // there
							  daw::invoke( callback, args... );
							  return callback.remove_after_run( );
						  } );
					}

				public:
					explicit basic_event_emitter( size_t max_listeners )
					  : m_max_listeners( max_listeners ) {}

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
						auto pos = listeners( ).find( event_name );
						if( pos == std::end( listeners( ) ) ) {
							return 0;
						}
						return pos->second.size( );
					}

					template<typename... ExpectedArgs, typename Listener>
					callback_id_t add_listener(
					  daw::string_view event, Listener &&listener,
					  callback_run_mode_t run_mode = callback_run_mode_t::run_many ) {

						static_assert( std::is_invocable_v<Listener, ExpectedArgs...> or
						                 std::is_invocable_v<Listener>,
						               "Listener does not accept expected arguments" );

						daw::exception::precondition_check(
						  !event.empty( ), "Empty event name passed to add_listener" );
						daw::exception::precondition_check(
						  !at_max_listeners( event ), "Max listeners reached for event" );

						auto const callback_obj =
						  ee_impl::make_listener_t<Listener, ExpectedArgs...>( );

						auto callback = callback_info_t(
						  callback_obj.create( std::forward<Listener>( listener ) ),
						  callback_obj.arity, run_mode );

						auto callback_id = callback.id( );
						if( event != "newListener" ) {
							emit_listener_added( event, callback_id );
						}
						get_callbacks_for( event ).push_back( daw::move( callback ) );
						return callback_id;
					}

					template<typename... Args>
					void emit( daw::string_view event, Args &&... args ) {
						daw::exception::daw_throw_on_true(
						  event.empty( ), "Empty event name passed to emit" );
						auto const oe = daw::on_scope_exit( [&]( ) { --m_emit_depth; } );
						daw::exception::precondition_check(
						  ++m_emit_depth <= c_max_emit_depth,
						  "Max callback depth reached.  Possible loop" );

						emit_impl( event, std::forward<Args>( args )... );
						// If a self destruct event is tagged to this event, call it now so
						// that resources can be released
						auto const event_selfdestruct =
						  daw::fmt( "{0}_selfdestruct", event );
						if( m_listeners.count( event_selfdestruct ) > 0 ) {
							emit_impl(
							  event_selfdestruct ); // Called by self destruct code and must
							                        // be last so lifetime is controlled
						}
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
				};
			} // namespace ee_impl

			class StandardEventEmitter {
				using emitter_t =
				  ee_impl::basic_event_emitter<ee_impl::DefaultMaxEventCount>;
				std::shared_ptr<emitter_t> m_emitter =
				  std::make_shared<emitter_t>( 10 );

			public:
				using callback_id_t = ee_impl::callback_info_t::callback_id_t;

				StandardEventEmitter( ) = default;
				explicit StandardEventEmitter( size_t max_listeners );

				void remove_all_callbacks( daw::string_view event );
				size_t &max_listeners( ) noexcept;
				size_t const &max_listeners( ) const noexcept;
				size_t listener_count( daw::string_view event_name ) const;

				template<typename... ExpectedArgs, typename Listener>
				callback_id_t add_listener(
				  daw::string_view event, Listener &&listener,
				  callback_run_mode_t run_mode = callback_run_mode_t::run_many ) {

					return m_emitter->template add_listener<ExpectedArgs...>(
					  event, std::forward<Listener>( listener ), run_mode );
				}

				template<typename... Args>
				void emit( daw::string_view event, Args &&... args ) {
					m_emitter->emit( event, std::forward<Args>( args )... );
				}

				bool is_same_instance( StandardEventEmitter const &em ) const;

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

			template<typename... EventArgs, typename EventEmitter, typename Listener>
			constexpr decltype( auto ) add_listener( daw::string_view name,
			                                         EventEmitter &emitter,
			                                         Listener &&listener ) {

				return emitter.template add_listener<EventArgs...>(
				  name, std::forward<Listener>( listener ),
				  callback_run_mode_t::run_many );
			}

			template<typename... EventArgs, typename EventEmitter, typename Listener>
			constexpr decltype( auto ) add_listener(
			  daw::string_view name, EventEmitter &&emitter, Listener &&listener,
			  callback_run_mode_t run_mode = callback_run_mode_t::run_many ) {

				return emitter.template add_listener<EventArgs...>(
				  name, std::forward<Listener>( listener ), run_mode );
			}

			//////////////////////////////////////////////////////////////////////////
			// Allows one to have the Events defined in event emitter
			template<typename Derived, typename EventEmitter>
			struct BasicStandardEvents {
				using callback_id_t = ee_impl::callback_info_t::callback_id_t;
				using event_emitter_t = EventEmitter;

			protected:
				event_emitter_t m_emitter{};

				constexpr Derived &child( ) noexcept {
					return *reinterpret_cast<Derived *>( this );
				}

				constexpr Derived const &child( ) const noexcept {
					return *reinterpret_cast<Derived const *>( this );
				}

				void emit_error( base::Error error ) {
					m_emitter.emit_error( daw::move( error ) );
				}

				void detect_delegate_loops( event_emitter_t const &em ) const {
					if( emitter( ).is_same_instance( em ) ) {
						daw::exception::daw_throw(
						  "Attempt to delegate to self.  This is a callback loop" );
					}
				}

				BasicStandardEvents( ) = default;

			public:
				explicit BasicStandardEvents( event_emitter_t &&emitter )
				  : m_emitter( daw::move( emitter ) ) {}

				explicit BasicStandardEvents( event_emitter_t const &emitter )
				  : m_emitter( emitter ) {}

				event_emitter_t &emitter( ) {
					return m_emitter; // If you get a warning about a recursive call here,
					                  // you forgot to create a emitter() in Derived
				}

				event_emitter_t const &emitter( ) const {
					return m_emitter; // If you get a warning about a recursive call here,
					                  // you forgot to create a emitter() in Derived
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is for when error's occur
				template<typename Listener>
				Derived &on_error( Listener &&listener ) {
					add_listener<base::Error>( "error", m_emitter,
					                           std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is for the next error
				template<typename Listener>
				Derived &on_next_error( Listener &&listener ) {
					add_listener<base::Error>( "error", m_emitter,
					                           std::forward<Listener>( listener ),
					                           callback_run_mode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called whenever a new listener is added for
				///				any callback
				template<typename Listener>
				Derived &on_listener_added( Listener &&listener ) {
					add_listener<std::string, callback_id_t>(
					  "listener_added", m_emitter, std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called when the next new listener is added
				///				for any callback
				template<typename Listener>
				Derived &on_next_listener_added( Listener &&listener ) {
					add_listener<std::string, callback_id_t>(
					  "listener_added", m_emitter, std::forward<Listener>( listener ),
					  callback_run_mode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is called whenever a listener is removed for
				/// any callback
				template<typename Listener>
				Derived &on_listener_removed( Listener &&listener ) {
					add_listener<std::string, callback_id_t>(
					  "listener_removed", m_emitter, std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Callback is called the next time a listener is removed for
				/// any callback
				template<typename Listener>
				Derived &on_next_listener_removed( Listener &&listener ) {
					add_listener<std::string, callback_id_t>(
					  "listener_removed", m_emitter, std::forward<Listener>( listener ),
					  callback_run_mode_t::run_once );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called when the subscribed object is exiting.
				///				This does not necessarily, but can be, from it's
				///				destructor.  Make sure to wrap in try/catch if in
				///				destructor
				template<typename Listener>
				Derived &on_exit( Listener &&listener ) {
					add_listener<std::optional<Error>>(
					  "exit", m_emitter, std::forward<Listener>( listener ) );
					return child( );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Callback is called when the subscribed object is exiting.
				///				This does not necessarily, but can be, from it's
				///				destructor.  Make sure to wrap in try/catch if in
				///				destructor
				template<typename Listener>
				Derived &on_next_exit( Listener &&listener ) {
					add_listener<std::optional<Error>>(
					  "exit", m_emitter, std::forward<Listener>( listener ),
					  callback_run_mode_t::run_once );
					return child( );
				}

				/// @brief Delegate error callbacks to another error handler
				/// @param error_destination A weak_ptr to destination object
				/// @param description Possible description of error
				/// @param where Where on_error was called from
				Derived &on_error( StandardEventEmitter error_destination,
				                   std::string description, std::string where ) {
					on_error( [error_destination =
					             mutable_capture( daw::move( error_destination ) ),
					           description = daw::move( description ),
					           where = daw::move( where )]( base::Error error ) {
						error_destination->emit_error( daw::move( error ), description,
						                               where );
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

				template<typename BasicStandardEventsChild>
				Derived &on_error( daw::observable_ptr<BasicStandardEventsChild>
				error_destination, std::string description, std::string where ) { return
				on_error( std::weak_ptr<BasicStandardEventsChild>( error_destination ),
				daw::move( description ), daw::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Delegate error callbacks to another error handler
				/// @param error_destination Object with emitter to send events too
				/// @param description Possible description of error
				/// @param where Where on_error was called from
				template<typename BasicStandardEventsChild>
				Derived &on_error( BasicStandardEvents<StandardEventsChild>
				&error_destination, std::string description, std::string where ) {
				  on_error( [ obj = mutable_capture( error_destination.obs_emiter( ) ),
				description, where
				]( base::Error const &error ) { obj.lock( [&]( auto &self ) {
				self.emit( "error", error, description, where ); } ); } ); return child(
				);
				}
				*/
				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( std::string description, std::string where ) {
					m_emitter.emit_error( daw::move( description ), daw::move( where ) );
				}

				/// @brief Emit an error event
				/// @param child A child error event to get a stack trace of errors
				/// @param description Description text
				/// @param where where in code error happened
				void emit_error( base::Error const &child, std::string description,
				                 std::string where ) {
					m_emitter.emit_error( child, daw::move( description ),
					                      daw::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( ErrorCode const &error, std::string description,
				                 std::string where ) {
					m_emitter.emit_error( error, daw::move( description ),
					                      daw::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief Emit an error event
				void emit_error( std::exception_ptr ex, std::string description,
				                 std::string where ) {
					m_emitter.emit_error( ex, daw::move( description ),
					                      daw::move( where ) );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit an event with the callback and event name of a newly
				///				added event
				void emit_listener_added( std::string event,
				                          callback_id_t callback_id ) {
					emitter( ).emit_listener_added( daw::move( event ), callback_id );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit an event with the callback and event name of an event
				///				that has been removed
				void emit_listener_removed( std::string event,
				                            callback_id_t callback_id ) {
					emitter( ).emit( "listener_removed", daw::move( event ),
					                 callback_id );
				}

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Emit and event when exiting to alert others that they
				///				may want to stop and exit. This version allows for an
				///				error reason
				void emit_exit( Error error ) {
					m_emitter.emit( "exit", create_optional_error( daw::move( error ) ) );
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
				static std::optional<ResultType>
				emit_error_on_throw( StandardEventEmitter &em,
				                     daw::string_view err_description,
				                     daw::string_view where, Func &&func ) {
					static_assert( std::is_invocable_v<Func>,
					               "Func must be callable without arguments" );
					try {
						return func( );
					} catch( ... ) {
						em.emit_error( std::current_exception( ), err_description, where );
						return std::nullopt;
					}
				}

				template<typename Class, typename Func>
				static void run_if_valid( std::weak_ptr<Class> obj,
				                          daw::string_view err_description,
				                          daw::string_view where, Func &&func ) {
					static_assert( std::is_invocable_v<Func, decltype( obj.lock( ) )>,
					               "Func must be callable with shared_ptr<Class>" );

					if( !obj.expired( ) ) {
						auto self = obj.lock( );
						emit_error_on_throw(
						  self, err_description, where,
						  [func = mutable_capture( std::forward<Func>( func ) ),
						   self = mutable_capture( self )]( ) {
							  daw::invoke( *func, *self );
						  } );
					}
				}

				template<typename... Args>
				Derived &delegate_to( daw::string_view source_event,
				                      StandardEventEmitter &em,
				                      std::string destination_event ) {
					detect_delegate_loops( em );
					m_emitter.template add_listener<Args...>(
					  source_event, [em = mutable_capture( em ),
					                 destination_event = daw::move( destination_event )](
					                  Args &&... args ) {
						  em->emit( destination_event, std::forward<Args>( args )... );
					  } );
					return child( );
				}
			}; // class BasicStandardEvents

			template<typename Derived>
			using StandardEvents = BasicStandardEvents<Derived, StandardEventEmitter>;
		} // namespace base
	}   // namespace nodepp
} // namespace daw
