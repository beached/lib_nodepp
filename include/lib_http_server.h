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
					class HttpServerImpl;
				}
				using HttpServer = std::shared_ptr<impl::HttpServerImpl>;

				HttpServer
				create_http_server( daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

				HttpServer
				create_http_server( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				                    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

				namespace impl {
					//////////////////////////////////////////////////////////////////////////
					// Summary:		An HTTP Server class
					// Requires:	daw::nodepp::lib::net::NetServer
					class HttpServerImpl : public daw::nodepp::base::enable_shared<HttpServerImpl>,
					                       public daw::nodepp::base::StandardEvents<HttpServerImpl> {
						daw::nodepp::lib::net::NetServer m_netserver;
						std::list<HttpServerConnection> m_connections;

						static void handle_connection( std::weak_ptr<HttpServerImpl> obj,
						                               daw::nodepp::lib::net::NetSocketStream socket );

						HttpServerImpl( daw::nodepp::base::EventEmitter emitter );

						HttpServerImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
						                daw::nodepp::base::EventEmitter emitter );

						friend daw::nodepp::lib::http::HttpServer
						daw::nodepp::lib::http::create_http_server( daw::nodepp::base::EventEmitter emitter );

						friend daw::nodepp::lib::http::HttpServer
						daw::nodepp::lib::http::create_http_server( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
						                                            daw::nodepp::base::EventEmitter emitter );

					public:
						~HttpServerImpl( ) override;

						HttpServerImpl( HttpServerImpl const & ) = default;
						HttpServerImpl( HttpServerImpl && ) noexcept = default;
						HttpServerImpl &operator=( HttpServerImpl const & ) = default;
						HttpServerImpl &operator=( HttpServerImpl && ) noexcept = default;

						void listen_on( uint16_t port,
						                daw::nodepp::lib::net::ip_version ip_ver = daw::nodepp::lib::net::ip_version::ipv4_v6,
						                uint16_t max_backlog = 511 );

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

						///
						/// \param listener - a callback that takes a HttpServerConnection as it's argument
						/// \return - a reference to *this
						template<typename Listener>
						HttpServerImpl &on_next_connected( Listener listener ) {
							emitter( )->template add_listener<HttpServerConnection>( "client_connected", std::move( listener ) );
							return *this;
						}

						template<typename Listener>
						HttpServerImpl &on_next_client_connected( Listener listener ) {
							emitter( )->template add_listener<HttpServerConnection>( "client_connected", std::move( listener ), callback_runmode_t::run_once );
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
					}; // class Server
				}    // namespace impl
			}      // namespace http
		}        // namespace lib
	}          // namespace nodepp
} // namespace daw
