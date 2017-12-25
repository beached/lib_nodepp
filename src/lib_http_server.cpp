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

#include <cinttypes>
#include <cstdlib>
#include <iterator>
#include <string>
#include <utility>

#include <daw/daw_exception.h>
#include <daw/daw_range_algorithm.h>
#include <daw/daw_utility.h>

#include "base_event_emitter.h"
#include "base_service_handle.h"
#include "lib_http_connection.h"
#include "lib_http_server.h"
#include "lib_net_server.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				HttpServer::HttpServer( base::EventEmitter emitter )
				  : daw::nodepp::base::StandardEvents<HttpServer>{std::move( emitter )}
				  , m_netserver{lib::net::NetServer{}} {}

				HttpServer::HttpServer( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				                        daw::nodepp::base::EventEmitter emitter )
				  : daw::nodepp::base::StandardEvents<HttpServer>{std::move( emitter )}
				  , m_netserver{lib::net::NetServer{ssl_config}} {}

				void HttpServer::emit_client_connected( HttpServerConnection connection ) {
					emitter( )->emit( "client_connected", std::move( connection ) );
				}

				void HttpServer::emit_closed( ) {
					emitter( )->emit( "closed" );
				}

				void HttpServer::emit_listening( daw::nodepp::lib::net::EndPoint endpoint ) {
					emitter( )->emit( "listening", std::move( endpoint ) );
				}

				void HttpServer::handle_connection( HttpServer self, lib::net::NetSocketStream socket ) {
					try {
						if( !socket || !( socket->is_open( ) ) || socket->is_closed( ) ) {
							self.emit_error( "Invalid socket passed to handle_connection", "HttpServer::handle_connection" );
							return;
						}
						HttpServerConnection connection{std::move( socket )};

						auto it = self.m_connections.emplace( self.m_connections.end( ), connection );

						connection.on_error( self, "Connection Error", "HttpServer::handle_connection" )
						  .on_closed( [it, self]( ) mutable {
							  try {
								  self.m_connections.erase( it );
							  } catch( ... ) {
								  self.emit_error( std::current_exception( ), "Could not delete connection",
								                   "HttpServer::handle_connection" );
							  }
						  } )
						  .start( );

						try {
							self.emit_client_connected( std::move( connection ) );
						} catch( ... ) {
							self.emit_error( std::current_exception( ), "Running connection listeners",
							                 "HttpServer::handle_connection" );
						}
					} catch( ... ) {
						self.emit_error( std::current_exception( ), "Exception while connecting", "HttpServer::handle_connection" );
					}
				}

				void HttpServer::listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver, uint16_t max_backlog ) {
					try {
						HttpServer self{*this};
						m_netserver
						  .on_connection(
						    [self]( lib::net::NetSocketStream socket ) mutable { handle_connection( self, std::move( socket ) ); } )
						  .on_error( self, "Error listening", "HttpServer::listen_on" )
						  .template delegate_to<daw::nodepp::lib::net::EndPoint>( "listening", self, "listening" )
						  .listen( port, ip_ver, max_backlog );
					} catch( ... ) { emit_error( std::current_exception( ), "Error while listening", "HttpServer::listen_on" ); }
				}

				void HttpServer::listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver ) {
					try {
						HttpServer self{*this};
						m_netserver
						  .on_connection(
						    [self]( lib::net::NetSocketStream socket ) mutable { handle_connection( self, std::move( socket ) ); } )
						  .on_error( self, "Error listening", "HttpServer::listen_on" )
						  .template delegate_to<daw::nodepp::lib::net::EndPoint>( "listening", self, "listening" )
						  .listen( port, ip_ver );
					} catch( ... ) { emit_error( std::current_exception( ), "Error while listening", "HttpServer::listen_on" ); }
				}

				void HttpServer::listen_on( uint16_t port ) {
					listen_on( port, daw::nodepp::lib::net::ip_version::ipv6 );
				}

				size_t &HttpServer::max_header_count( ) {
					daw::exception::daw_throw_not_implemented( );
				}

				size_t const &HttpServer::max_header_count( ) const {
					daw::exception::daw_throw_not_implemented( );
				}

				size_t HttpServer::timeout( ) const {
					daw::exception::daw_throw_not_implemented( );
				}

				HttpServer::~HttpServer( ) = default;
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
