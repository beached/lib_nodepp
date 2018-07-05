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

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <utility>

#include <daw/daw_exception.h>
#include <daw/daw_range_algorithm.h>

#include "base_event_emitter.h"
#include "base_service_handle.h"
#include "base_types.h"
#include "lib_net_server.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using namespace daw::nodepp;
				using namespace boost::asio::ip;

				NetNoSslServer::NetNoSslServer( base::EventEmitter &&emitter )
				  : daw::nodepp::base::StandardEvents<NetNoSslServer>(
				      std::move( emitter ) )
				  , m_acceptor(
				      daw::make_observable_ptr_pair<boost::asio::ip::tcp::acceptor>(
				        base::ServiceHandle::get( ) ) ) {}

				NetNoSslServer::NetNoSslServer( base::EventEmitter const &emitter )
				  : daw::nodepp::base::StandardEvents<NetNoSslServer>( emitter )
				  , m_acceptor(
				      daw::make_observable_ptr_pair<boost::asio::ip::tcp::acceptor>(
				        base::ServiceHandle::get( ) ) ) {}

				void NetNoSslServer::listen( uint16_t port, ip_version ip_ver,
				                             uint16_t max_backlog ) {
					try {
						auto const tcp = ip_ver == ip_version::ipv4
						                   ? boost::asio::ip::tcp::v4( )
						                   : boost::asio::ip::tcp::v6( );
						EndPoint endpoint{tcp, port};
						m_acceptor.visit( [&]( auto &ac ) {
							ac.open( endpoint.protocol( ) );
							ac.set_option(
							  boost::asio::ip::tcp::acceptor::reuse_address{true} );
							set_ipv6_only( ac, ip_ver );
							ac.bind( endpoint );
							ac.listen( max_backlog );
						} );
						start_accept( );
						emitter( ).emit( "listening", std::move( endpoint ) );
					} catch( ... ) {
						emit_error( std::current_exception( ),
						            "Error listening for connection",
						            "NetNoSslServer::listen" );
					}
				}

				void NetNoSslServer::listen( uint16_t port, ip_version ip_ver ) {
					try {
						auto const tcp = ip_ver == ip_version::ipv4
						                   ? boost::asio::ip::tcp::v4( )
						                   : boost::asio::ip::tcp::v6( );
						EndPoint endpoint{tcp, port};
						m_acceptor.visit( [&]( auto &ac ) {
							ac.open( endpoint.protocol( ) );
							ac.set_option(
							  boost::asio::ip::tcp::acceptor::reuse_address{true} );
							set_ipv6_only( ac, ip_ver );
							ac.bind( endpoint );
							ac.listen( );
						} );
						start_accept( );
						emitter( ).emit( "listening", std::move( endpoint ) );
					} catch( ... ) {
						emit_error( std::current_exception( ),
						            "Error listening for connection",
						            "NetNoSslServer::listen" );
					}
				}

				void NetNoSslServer::listen( uint16_t port ) {
					listen( port, ip_version::ipv6 );
				}

				void NetNoSslServer::close( ) {
					daw::exception::daw_throw_not_implemented( );
				}

				NetAddress NetNoSslServer::address( ) const {
					std::stringstream ss{};
					ss << m_acceptor->local_endpoint( );
					return NetAddress{ss.str( )};
				}

				void NetNoSslServer::handle_accept( NetNoSslServer &self,
				                                    NetSocketStream socket,
				                                    base::ErrorCode err ) {
					try {
						if( err.value( ) == 24 ) {
							self.emit_error( err, "Too many open files",
							                 "NetNoSslServer::handle_accept" );
						} else {
							daw::exception::daw_throw_value_on_true( err );
							self.emitter( ).emit( "connection", std::move( socket ) );
						}
					} catch( ... ) {
						self.emit_error( std::current_exception( ),
						                 "Exception while accepting connections",
						                 "NetNoSslServer::handle_accept" );
					}
					self.start_accept( );
				}

				void NetNoSslServer::start_accept( ) {
					try {
						NetSocketStream socket{};
						m_acceptor->async_accept(
						  socket.socket( )->next_layer( ),
						  [self = *this,
						   socket = std::move( socket )]( base::ErrorCode err ) mutable {
							  handle_accept( self, socket, std::move( err ) );
						  } );
					} catch( ... ) {
						emit_error( std::current_exception( ),
						            "Error while starting accept",
						            "NetNoSslServer::start_accept" );
					}
				}
			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
