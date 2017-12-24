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

#include <boost/variant.hpp>

#include "lib_net_nossl_server.h"
#include "lib_net_ssl_server.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				namespace impl {
					/// @brief		A TCP Server class
					///
					class NetServerImpl : public daw::nodepp::base::enable_shared<NetServerImpl>,
					                      public daw::nodepp::base::StandardEvents<NetServerImpl> {
						using NetNoSslServer = std::shared_ptr<NetNoSslServerImpl>;
						using NetSslServer = std::shared_ptr<NetSslServerImpl>;
						using value_type = boost::variant<NetNoSslServer, NetSslServer>;
						value_type m_net_server;

					public:
						NetServerImpl( daw::nodepp::base::EventEmitter emitter );

						NetServerImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
						               daw::nodepp::base::EventEmitter emitter );

						NetServerImpl( ) = delete;
						~NetServerImpl( ) override;
						NetServerImpl( NetServerImpl const & ) = default;
						NetServerImpl( NetServerImpl && ) noexcept = default;
						NetServerImpl &operator=( NetServerImpl const & ) = default;
						NetServerImpl &operator=( NetServerImpl && ) noexcept = default;

						bool using_ssl( ) const noexcept;

						void listen( uint16_t port );
						void listen( uint16_t port, ip_version ip_ver );
						void listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog );

						void close( );

						daw::nodepp::lib::net::NetAddress address( ) const;

						void get_connections( std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback );

						template<typename Listener>
						void on_connection( Listener listener ) {
							emitter( )->template add_listener<NetSocketStream>( "connection", std::move( listener ) );
						}

						template<typename Listener>
						void on_next_connection( Listener listener ) {
							emitter( )->template add_listener<NetSocketStream>( "connection", std::move( listener ),
							                                                    callback_runmode_t::run_once );
						}

						template<typename Listener>
						void on_listening( Listener listener ) {
							emitter( )->template add_listener<EndPoint>( "listening", std::move( listener ) );
						}

						template<typename Listener>
						void on_next_listening( Listener listener ) {
							emitter( )->template add_listener<EndPoint>( "listening", std::move( listener ), callback_runmode_t::run_once );
						}

						template<typename Listener>
						void on_closed( Listener listener ) {
							emitter( )->template add_listener<>( "closed", std::move( listener ), callback_runmode_t::run_once );
						}

						void emit_connection( NetSocketStream socket );
						void emit_listening( EndPoint endpoint );
						void emit_closed( );
					}; // class NetServerImpl
				}    // namespace impl

				class NetServer {
					std::shared_ptr<impl::NetServerImpl> m_net_server;

				public:
					NetServer( daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

					NetServer( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					           daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

					~NetServer( ) = default;
					NetServer( NetServer const & ) = default;
					NetServer( NetServer && ) noexcept = default;
					NetServer &operator=( NetServer const & ) = default;
					NetServer &operator=( NetServer && ) noexcept = default;

					impl::NetServerImpl const &base( ) const;
					impl::NetServerImpl &base( );
					explicit operator bool( ) const noexcept;

					bool using_ssl( ) const noexcept;

					void listen( uint16_t port );
					void listen( uint16_t port, ip_version ip_ver );
					void listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog );

					void close( );

					daw::nodepp::lib::net::NetAddress address( ) const;

					void get_connections( std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback );

					// Event callbacks

					/// @brief	Event emitted when a connection is established
					///
					NetServer &on_connection( std::function<void(NetSocketStream)> listener );

					/// @brief	Event emitted when a connection is established
					///
					NetServer &on_next_connection( std::function<void(NetSocketStream)> listener );

					/// @brief	Event emitted when the server is bound after calling
					/// listen( ... )
					///
					NetServer &on_listening( std::function<void( EndPoint )> listener );
					/// @brief	Event emitted when the server is bound after calling
					/// listen( ... )
					///
					NetServer &on_next_listening( std::function<void(EndPoint)> listener );

					/// @brief	Event emitted when the server closes and all connections
					/// are closed
					///
					NetServer &on_closed( std::function<void()> listener );

					/// @brief	Event emitted when a connection is established
					///
					void emit_connection( NetSocketStream socket );

					/// @brief	Event emitted when the server is bound after calling
					///				listen( ... )
					///
					void emit_listening( EndPoint endpoint );

					/// @brief	Event emitted when the server is bound after calling
					///				listen( ... )
					///
					void emit_closed( );

					template<typename Listener>
					NetServer &on_error( Listener &&listener ) {
						m_net_server->on_error( std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					NetServer &on_next_error( Listener &&listener ) {
						m_net_server->on_next_error( std::forward<Listener>( listener ) );
						return *this;
					}

					base::EventEmitter &emitter( );
					base::EventEmitter const &emitter( ) const;

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Callback is called when the subscribed object is exiting.
					///				This does not necessarily, but can be, from it's
					///				destructor.  Make sure to wrap in try/catch if in
					///				destructor
					template<typename Listener>
					NetServer &on_exit( Listener listener ) {
						m_net_server->on_exit( std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Callback is called when the subscribed object is exiting.
					///				This does not necessarily, but can be, from it's
					///				destructor.  Make sure to wrap in try/catch if in
					///				destructor
					template<typename Listener>
					NetServer &on_next_exit( Listener listener ) {
						m_net_server->on_next_exit( std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Emit an error event
					void emit_error( std::string description, std::string where );

					//////////////////////////////////////////////////////////////////////////
					/// @brief Emit an error event
					void emit_error( base::Error const &child, std::string description, std::string where );

					//////////////////////////////////////////////////////////////////////////
					/// @brief Emit an error event
					void emit_error( base::ErrorCode const &error, std::string description, std::string where );

					//////////////////////////////////////////////////////////////////////////
					/// @brief Emit an error event
					void emit_error( std::exception_ptr ex, std::string description, std::string where );

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Emit and event when exiting to alert others that they
					///				may want to stop and exit. This version allows for an
					///				error reason
					void emit_exit( base::Error error );

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Emit and event when exiting to alert others that they
					///				may want to stop and exit.
					void emit_exit( );

					//////////////////////////////////////////////////////////////////////////
					/// Delegate error callbacks to another error handler
					///
					/// @param error_destination A weak_ptr to destination object
					/// @param description Possible description of error
					/// @param where Where on_error was called from
					template<typename StandardEventsChild>
					NetServer &on_error( std::weak_ptr<StandardEventsChild> error_destination, std::string description,
					                     std::string where ) {
						m_net_server->on_error( error_destination, description, where );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// Delegate error callbacks to another error handler
					///
					/// @param error_destination A shared_ptr to destination object
					/// @param description Possible description of error
					/// @param where Where on_error was called from
					template<typename StandardEventsChild>
					NetServer &on_error( std::shared_ptr<StandardEventsChild> error_destination, std::string description,
					                     std::string where ) {
						m_net_server->on_error( std::weak_ptr<StandardEventsChild>( error_destination ), std::move( description ),
						                        std::move( where ) );

						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Creates a callback on the event source that calls a
					///				mirroring emitter on the destination obj. Unless the
					///				callbacks are of the form std::function<void( )> the
					///				callback parameters must be template parameters here.
					///				e.g.
					///				obj_emitter.delegate_to<daw::nodepp::lib::net::EndPoint>( "listening",
					/// dest_obj.get_weak_ptr(
					///), "listening" );
					template<typename... Args, typename DestinationType>
					NetServer &delegate_to( daw::string_view source_event, std::weak_ptr<DestinationType> destination_obj,
					                        std::string destination_event ) {
						m_net_server->delegate_to( source_event, std::move( destination_obj ), std::move( destination_event ) );
						return *this;
					}
				};

			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
