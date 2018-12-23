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

#include <asio/ip/tcp.hpp>
#include <list>
#include <memory>
#include <string>
#include <type_traits>

#include <daw/json/daw_json_link.h>

#include "base_error.h"
#include "base_event_emitter.h"
#include "base_service_handle.h"
#include "base_types.h"
#include "lib_net_address.h"
#include "lib_net_server.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				/// @brief		A TCP Server class
				/// Requires:	daw::nodepp::base::EventEmitter,
				/// daw::nodepp::base::options_t,
				///				daw::nodepp::lib::net::NetAddress, daw::nodepp::base::Error
				template<typename EventEmitter>
				class NetSslServer
				  : public base::BasicStandardEvents<NetSslServer<EventEmitter>,
				                                     EventEmitter> {

					std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;
					SslServerConfig m_config;

					using base::BasicStandardEvents<NetSslServer<EventEmitter>, EventEmitter>::emitter;
					using base::BasicStandardEvents<NetSslServer<EventEmitter>, EventEmitter>::emit_error;
				public:
					NetSslServer( net::SslServerConfig ssl_config,
					              EventEmitter &&emit )
					  : base::BasicStandardEvents<NetSslServer, EventEmitter>(
					      daw::move( emit ) )
					  , m_acceptor( std::make_shared<asio::ip::tcp::acceptor>(
					      base::ServiceHandle::get( ) ) )
					  , m_config( daw::move( ssl_config ) ) {}

					NetSslServer( net::SslServerConfig ssl_config,
					              EventEmitter const &emit )
					  : base::BasicStandardEvents<NetSslServer, EventEmitter>( emit )
					  , m_acceptor( std::make_shared<asio::ip::tcp::acceptor>(
					      base::ServiceHandle::get( ) ) )
					  , m_config( daw::move( ssl_config ) ) {}

					void listen( uint16_t port, ip_version ip_ver,
					             uint16_t max_backlog ) {
						try {
							auto const tcp = ip_ver == ip_version::ipv4
							                   ? asio::ip::tcp::v4( )
							                   : asio::ip::tcp::v6( );
							auto endpoint = EndPoint( tcp, port );
							m_acceptor->open( endpoint.protocol( ) );
							m_acceptor->set_option(
							  asio::ip::tcp::acceptor::reuse_address( true ) );
							set_ipv6_only( *m_acceptor, ip_ver );
							m_acceptor->bind( endpoint );
							m_acceptor->listen( max_backlog );
							start_accept( );
							emitter( ).emit( "listening", daw::move( endpoint ) );
						} catch( ... ) {
							emit_error( std::current_exception( ),
							                  "Error listening for connection", "listen" );
						}
					}

					void listen( uint16_t port, ip_version ip_ver ) {
						try {
							auto const tcp = ip_ver == ip_version::ipv4
							                   ? asio::ip::tcp::v4( )
							                   : asio::ip::tcp::v6( );
							auto endpoint = EndPoint( tcp, port );
							m_acceptor->open( endpoint.protocol( ) );
							m_acceptor->set_option(
							  asio::ip::tcp::acceptor::reuse_address( true ) );
							set_ipv6_only( *m_acceptor, ip_ver );
							m_acceptor->bind( endpoint );
							m_acceptor->listen( );
							start_accept( );
							emitter( ).emit( "listening", daw::move( endpoint ) );
						} catch( ... ) {
							emit_error( std::current_exception( ),
							                  "Error listening for connection", "listen" );
						}
					}

					void listen( uint16_t port ) {
						listen( port, ip_version::ipv6 );
					}
					void close( ) {
						daw::exception::daw_throw_not_implemented( );
					}

					NetAddress address( ) const {
						std::stringstream ss{};
						ss << m_acceptor->local_endpoint( );
						return NetAddress{ss.str( )};
					}

					template<typename Listener>
					void get_connections( Listener && ) {
						static_assert( std::is_invocable_v<Listener, base::Error, uint16_t>,
						               "callback must be of the form ( base::Error err, "
						               "uint16_t count )" );

						daw::exception::daw_throw_not_implemented( );
					}

				private:
					static void handle_handshake( NetSslServer &self,
					                              NetSocketStream<EventEmitter> socket,
					                              base::ErrorCode err ) {

						daw::exception::daw_throw_value_on_true( err );
						self.emitter( ).emit( "connection", daw::move( socket ) );
					}

					static void handle_accept( NetSslServer &self,
					                           NetSocketStream<EventEmitter> socket,
					                           base::ErrorCode err ) {
						try {
							if( err.value( ) == 24 ) {
								self.emit_error( err, "Too many open files",
								                 "NetNoSslServer::handle_accept" );
							} else {
								daw::exception::daw_throw_value_on_true( err );
								auto tmp_sock = socket;
								tmp_sock.socket( ).handshake_async(
								  asio::ssl::stream_base::server,
								  [socket = mutable_capture( daw::move( socket ) ),
								   self = mutable_capture( daw::move( self ) )](
								    base::ErrorCode const &err1 ) {
									  handle_handshake( *self, daw::move( *socket ), err1 );
								  } );
							}
						} catch( ... ) {
							self.emit_error( std::current_exception( ),
							                 "Error while handling accept",
							                 "NetSslServer::handle_accept" );
						}
						self.start_accept( );
					}

					void start_accept( ) {
						try {
							auto socket = NetSocketStream<EventEmitter>( m_config );
							daw::exception::daw_throw_on_false(
							  socket,
							  "NetSslServer::start_accept( ), Invalid socket - null" );

							socket.socket( ).init( );
							auto &asio_socket = socket.socket( );

							m_acceptor->async_accept(
							  asio_socket->lowest_layer( ),
							  [socket = mutable_capture( daw::move( socket ) ),
							   self = mutable_capture( *this )]( base::ErrorCode err ) {
								  daw::exception::daw_throw_value_on_true( err );
								  handle_accept( *self, daw::move( *socket ), err );
							  } );
						} catch( ... ) {
							emit_error( std::current_exception( ),
							                  "Error while starting accept",
							                  "NetSslServer::start_accept" );
						}
					}

				}; // class NetSslServer
			}    // namespace net
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
