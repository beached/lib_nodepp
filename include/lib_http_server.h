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

#include <list>
#include <memory>
#include <vector>

#include <daw/daw_exception.h>

#include "base_event_emitter.h"
#include "lib_http_connection.h"
#include "lib_http_server_response.h"
#include "lib_net_server.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					/// @brief		An HTTP Server class
					///
					class HttpServerImpl : public daw::nodepp::base::enable_shared<HttpServerImpl>,
					                       public daw::nodepp::base::StandardEvents<HttpServerImpl> {
						daw::nodepp::lib::net::NetServer m_netserver;
						std::list<HttpServerConnection> m_connections;

						static void handle_connection( std::weak_ptr<HttpServerImpl> obj,
						                               daw::nodepp::lib::net::NetSocketStream socket );

					public:
						explicit HttpServerImpl( daw::nodepp::base::EventEmitter emitter );

						HttpServerImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
						                daw::nodepp::base::EventEmitter emitter );

						~HttpServerImpl( ) override;

						HttpServerImpl( HttpServerImpl const & ) = default;

						HttpServerImpl( HttpServerImpl && ) noexcept = default;

						HttpServerImpl &operator=( HttpServerImpl const & ) = default;

						HttpServerImpl &operator=( HttpServerImpl && ) noexcept = default;

						void listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver, uint16_t max_backlog );

						void listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver );

						void listen_on( uint16_t port );

						size_t &max_header_count( );

						size_t const &max_header_count( ) const;

						template<typename Listener>
						void set_timeout( size_t msecs, Listener listener ) {
							daw::exception::daw_throw_not_implemented( );
						}

						template<typename Listener>
						HttpServerImpl &on_listening( Listener listener ) {
							emitter( )->template add_listener<daw::nodepp::lib::net::EndPoint>( "listening", std::move( listener ) );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_next_listening( Listener listener ) {
							emitter( )->template add_listener<daw::nodepp::lib::net::EndPoint>( "listening", std::move( listener ),
							                                                                    callback_runmode_t::run_once );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_next_connected( Listener listener ) {
							emitter( )->template add_listener<HttpServerConnection>( "client_connected", std::move( listener ),
							                                                         callback_runmode_t::run_once );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_client_connected( Listener listener ) {
							emitter( )->template add_listener<HttpServerConnection>( "client_connected", std::move( listener ) );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_next_client_connected( Listener listener ) {
							emitter( )->template add_listener<HttpServerConnection>( "client_connected", std::move( listener ),
							                                                         callback_runmode_t::run_once );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_closed( Listener listener ) {
							emitter( )->template add_listener<>( "closed", std::move( listener ) );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_next_closed( Listener listener ) {
							emitter( )->template add_listener<>( "closed", std::move( listener ), callback_runmode_t::run_once );
							return *this;
						}

						size_t timeout( ) const;

						void emit_client_connected( HttpServerConnection connection );

						void emit_closed( );

						void emit_listening( daw::nodepp::lib::net::EndPoint endpoint );
					};
				} // namespace impl

				/// @brief		An HTTP Server class
				///
				class HttpServer {
					std::shared_ptr<impl::HttpServerImpl> m_http_server;

				public:
					explicit HttpServer( daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

					explicit HttpServer( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					            daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

					~HttpServer( ) = default;
					HttpServer( HttpServer const & ) = default;
					HttpServer( HttpServer && ) noexcept = default;
					HttpServer &operator=( HttpServer const & ) = default;
					HttpServer &operator=( HttpServer && ) noexcept = default;

					void listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver, uint16_t max_backlog );
					void listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver );
					void listen_on( uint16_t port );

					size_t &max_header_count( );
					size_t const &max_header_count( ) const;

					size_t timeout( ) const;

					template<typename Listener>
					void set_timeout( size_t msecs, Listener listener ) {
						daw::exception::daw_throw_not_implemented( );
					}

					template<typename Listener>
					HttpServer &on_listening( Listener listener ) {
						m_http_server->on_listening( std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_listening( Listener listener ) {
						m_http_server->on_next_listening( std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_connected( Listener listener ) {
						m_http_server->on_next_connected( std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_client_connected( Listener listener ) {
						m_http_server->on_client_connected( std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_client_connected( Listener listener ) {
						m_http_server->on_next_client_connected( std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_closed( Listener listener ) {
						m_http_server->on_closed( std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_closed( Listener listener ) {
						m_http_server->on_next_closed( std::move( listener ) );
						return *this;
					}

					void emit_client_connected( HttpServerConnection connection );
					void emit_closed( );
					void emit_listening( net::EndPoint endpoint );

					/// @brief Callback is for when error's occur
					///
					template<typename Listener>
					HttpServer &on_error( Listener listener ) {
						m_http_server->on_error( std::move( listener ) );
						return *this;
					}

					/// @brief Callback is for the next error
					///
					HttpServer &on_next_error( std::function<void( base::Error )> listener ) {
						m_http_server->on_error( std::move( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Delegate error callbacks to another error handler
					/// @param error_destination A weak_ptr to destination object
					/// @param description Possible description of error
					/// @param where Where on_error was called from
					template<typename StandardEventsChild>
					HttpServer &on_error( std::weak_ptr<StandardEventsChild> error_destination, std::string description,
					                      std::string where ) {
						m_http_server->on_error( std::move( error_destination ), std::move( description ), std::move( where ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Delegate error callbacks to another error handler
					/// @param error_destination A shared_ptr to destination object
					/// @param description Possible description of error
					/// @param where Where on_error was called from
					template<typename StandardEventsChild>
					HttpServer &on_error( std::shared_ptr<StandardEventsChild> error_destination, std::string description,
					                      std::string where ) {

						m_http_server->on_error( std::move( error_destination ), std::move( description ), std::move( where ) );
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
					HttpServer &delegate_to( daw::string_view source_event, std::weak_ptr<DestinationType> destination_obj,
					                      std::string destination_event ) {
						m_http_server->delegate_to<Args...>( source_event, std::move( destination_obj ), std::move( destination_event ) );
						return *this;
					}
				};
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
