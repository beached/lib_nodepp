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
				//////////////////////////////////////////////////////////////////////////
				/// @brief		A TCP Server class
				// Requires:	daw::nodepp::EventEmitter,
				// daw::nodepp::base::options_t,
				//				daw::nodepp::lib::net::NetAddress, daw::nodepp::base::Error
				template<typename EventEmitter = base::StandardEventEmitter>
				class NetNoSslServer
				  : public base::BasicStandardEvents<NetNoSslServer<EventEmitter>,
				                                     EventEmitter> {

					std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;

					using base::BasicStandardEvents<NetNoSslServer<EventEmitter>,
					                                EventEmitter>::emitter;
					using base::BasicStandardEvents<NetNoSslServer<EventEmitter>,
					                                EventEmitter>::emit_error;

				public:
					explicit NetNoSslServer( EventEmitter emit )
					  : base::BasicStandardEvents<NetNoSslServer, EventEmitter>( emit )
					  , m_acceptor( std::make_shared<asio::ip::tcp::acceptor>(
					      base::ServiceHandle::get( ) ) ) {}

					void listen( uint16_t port, ip_version ip_ver,
					             uint16_t max_backlog ) {
						try {
							auto const tcp = ( ip_ver == ip_version::ipv4 )
							                   ? asio::ip::tcp::v4( )
							                   : asio::ip::tcp::v6( );
							auto endpoint = EndPoint( tcp, port );
							m_acceptor->open( endpoint.protocol( ) );
							m_acceptor->set_option(
							  asio::ip::tcp::acceptor::reuse_address{true} );
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

					template<bool NotImplemented = true>
					void close( ) {
						static_assert( !NotImplemented );
					}

					NetAddress address( ) const {
						auto ss = std::stringstream( );
						ss << m_acceptor->local_endpoint( );
						return NetAddress{ss.str( )};
					}

					template<typename Callback, bool NotImplemented = true>
					void get_connections( Callback && ) {
						static_assert( std::is_invocable_v<Callback, base::Error /*err*/,
						                                   uint16_t /*count*/>,
						               "Callback does not accept needed arguments" );
						static_assert( !NotImplemented );
					}

				private:
					static void handle_accept( NetNoSslServer &self,
					                           NetSocketStream<EventEmitter> socket,
					                           base::ErrorCode err ) {
						try {
							if( err.value( ) == 24 ) {
								self.emit_error( err, "Too many open files", "handle_accept" );
							} else {
								daw::exception::daw_throw_value_on_true( err );
								self.emitter( ).emit( "connection", std::move( socket ) );
							}
						} catch( ... ) {
							self.emit_error( std::current_exception( ),
							                 "Exception while accepting connections",
							                 "handle_accept" );
						}
						self.start_accept( );
					}

					void start_accept( ) {
						try {
							auto socket = NetSocketStream<EventEmitter>( );
							m_acceptor->async_accept(
							  socket.socket( )->next_layer( ),
							  [self = this,
							   socket = mutable_capture( socket )]( base::ErrorCode err ) {
								  handle_accept( *self, *socket, err );
							  } );
						} catch( ... ) {
							emit_error( std::current_exception( ),
							            "Error while starting accept", "start_accept" );
						}
					}
				}; // class NetNoSslServer
			}    // namespace net
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
