// The MIT License (MIT)
//
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
				/// @brief		An HTTP Server class
				///
				template<typename EventEmitter = base::StandardEventEmitter>
				class HttpServer
				  : public base::BasicStandardEvents<HttpServer<EventEmitter>,
				                                     EventEmitter> {

					net::NetServer<EventEmitter> m_netserver;
					std::list<HttpServerConnection<EventEmitter>> m_connections;

					static void
					handle_connection( HttpServer<EventEmitter> &self,
					                   net::NetSocketStream<EventEmitter> socket ) {
						try {
							if( !socket || !( socket.is_open( ) ) || socket.is_closed( ) ) {
								self.emit_error( "Invalid socket passed to handle_connection",
								                 "HttpServer::handle_connection" );
								return;
							}
							auto connection =
							  HttpServerConnection<EventEmitter>( std::move( socket ) );

							auto it = self.m_connections.emplace( self.m_connections.end( ),
							                                      connection );

							connection
							  .on_error( self.emitter( ), "Connection Error",
							             "HttpServer::handle_connection" )
							  .on_closed( [it = mutable_capture( it ),
							               self = mutable_capture( self )]( ) {
								  try {
									  self->m_connections.erase( *it );
								  } catch( ... ) {
									  self->emit_error( std::current_exception( ),
									                    "Could not delete connection",
									                    "HttpServer::handle_connection" );
								  }
							  } )
							  .start( );

							try {
								self.emit_client_connected( std::move( connection ) );
							} catch( ... ) {
								self.emit_error( std::current_exception( ),
								                 "Running connection listeners",
								                 "HttpServer::handle_connection" );
							}
						} catch( ... ) {
							self.emit_error( std::current_exception( ),
							                 "Exception while connecting",
							                 "HttpServer::handle_connection" );
						}
					}

				public:
					explicit HttpServer( EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<HttpServer<EventEmitter>, EventEmitter>(
					      std::move( emitter ) )
					  , m_netserver( net::NetServer<EventEmitter>( ) ) {}

					explicit HttpServer( net::SslServerConfig const &ssl_config,
					                     EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<HttpServer<EventEmitter>, EventEmitter>(
					      std::move( emitter ) )
					  , m_netserver( net::NetServer<EventEmitter>( ssl_config ) ) {}

					void listen_on( uint16_t port, net::ip_version ip_ver,
					                uint16_t max_backlog ) {
						try {
							m_netserver
							  .on_connection(
							    [self = this]( net::NetSocketStream<EventEmitter> socket ) {
								    handle_connection( *self, std::move( socket ) );
							    } )
							  .on_error( this->emitter( ), "Error listening",
							             "HttpServer::listen_on" )
							  .template delegate_to<net::EndPoint>(
							    "listening", this->emitter( ), "listening" )
							  .listen( port, ip_ver, max_backlog );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Error while listening",
							                  "HttpServer::listen_on" );
						}
					}

					void listen_on( uint16_t port, net::ip_version ip_ver ) {
						try {
							m_netserver
							  .on_connection(
							    [self = this]( net::NetSocketStream<EventEmitter> socket ) {
								    handle_connection( *self, std::move( socket ) );
							    } )
							  .on_error( this->emitter( ), "Error listening",
							             "HttpServer::listen_on" )
							  .template delegate_to<net::EndPoint>(
							    "listening", this->emitter( ), "listening" )
							  .listen( port, ip_ver );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Error while listening",
							                  "HttpServer::listen_on" );
						}
					}

					void listen_on( uint16_t port ) {
						listen_on( port, net::ip_version::ipv6 );
					}

					size_t &max_header_count( ) {
						daw::exception::daw_throw_not_implemented( );
					}

					size_t const &max_header_count( ) const {
						daw::exception::daw_throw_not_implemented( );
					}

					template<typename Listener>
					void set_timeout( size_t msecs, Listener && ) {
						daw::exception::daw_throw_not_implemented( );
					}

					template<typename Listener>
					HttpServer &on_listening( Listener &&listener ) {
						base::add_listener<net::EndPoint>(
						  "listening", this->emitter( ),
						  std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_listening( Listener &&listener ) {
						base::add_listener<net::EndPoint>(
						  "listening", this->emitter( ), std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_connected( Listener &&listener ) {
						base::add_listener<HttpServerConnection>(
						  "client_connected", this->emitter( ),
						  std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_client_connected( Listener &&listener ) {
						base::add_listener<HttpServerConnection<EventEmitter>>(
						  "client_connected", this->emitter( ),
						  std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_client_connected( Listener &&listener ) {
						base::add_listener<HttpServerConnection<EventEmitter>>(
						  "client_connected", this->emitter( ),
						  std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_closed( Listener &&listener ) {
						base::add_listener<>( "closed", this->emitter( ),
						                      std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					HttpServer &on_next_closed( Listener &&listener ) {
						base::add_listener<>( "closed", this->emitter( ),
						                      std::forward<Listener>( listener ),
						                      base::callback_run_mode_t::run_once );
						return *this;
					}

					size_t timeout( ) const {
						daw::exception::daw_throw_not_implemented( );
					}

					void emit_client_connected(
					  HttpServerConnection<EventEmitter> connection ) {
						this->emitter( ).emit( "client_connected",
						                       std::move( connection ) );
					}

					void emit_closed( ) {
						this->emitter( ).emit( "closed" );
					}

					void emit_listening( net::EndPoint endpoint ) {
						this->emitter( ).emit( "listening", std::move( endpoint ) );
					}
				};
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
