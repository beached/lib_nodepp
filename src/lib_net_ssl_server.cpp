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

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

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

					NetSslServer::NetSslServer( daw::nodepp::lib::net::SslServerConfig ssl_config,
					                                    daw::nodepp::base::EventEmitter emitter )
					  : daw::nodepp::base::StandardEvents<NetSslServer>{std::move( emitter )}
					  , m_acceptor{daw::nodepp::impl::make_shared_ptr<boost::asio::ip::tcp::acceptor>(
					      base::ServiceHandle::get( ) )}
					  , m_config{std::move( ssl_config )} {}

					NetSslServer::~NetSslServer( ) = default;

					void NetSslServer::listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog ) {
						try {
							auto const tcp = ip_ver == ip_version::ipv4 ? boost::asio::ip::tcp::v4( ) : boost::asio::ip::tcp::v6( );
							auto endpoint = EndPoint( tcp, port );
							m_acceptor->open( endpoint.protocol( ) );
							m_acceptor->set_option( boost::asio::ip::tcp::acceptor::reuse_address( true ) );
							impl::set_ipv6_only( m_acceptor, ip_ver );
							m_acceptor->bind( endpoint );
							m_acceptor->listen( max_backlog );
							start_accept( );
							emitter( )->emit( "listening", std::move( endpoint ) );
						} catch( ... ) {
							emit_error( std::current_exception( ), "Error listening for connection", "NetSslServer::listen" );
						}
					}

					void NetSslServer::listen( uint16_t port, ip_version ip_ver ) {
						try {
							auto const tcp = ip_ver == ip_version::ipv4 ? boost::asio::ip::tcp::v4( ) : boost::asio::ip::tcp::v6( );
							auto endpoint = EndPoint( tcp, port );
							m_acceptor->open( endpoint.protocol( ) );
							m_acceptor->set_option( boost::asio::ip::tcp::acceptor::reuse_address( true ) );
							impl::set_ipv6_only( m_acceptor, ip_ver );
							m_acceptor->bind( endpoint );
							m_acceptor->listen( );
							start_accept( );
							emitter( )->emit( "listening", std::move( endpoint ) );
						} catch( ... ) {
							emit_error( std::current_exception( ), "Error listening for connection", "NetSslServer::listen" );
						}
					}

					void NetSslServer::listen( uint16_t port ) {
						listen( port, ip_version::ipv6 );
					}

					void NetSslServer::close( ) {
						daw::exception::daw_throw_not_implemented( );
					}

					NetAddress NetSslServer::address( ) const {
						std::stringstream ss{};
						ss << m_acceptor->local_endpoint( );
						return NetAddress{ss.str( )};
					}

					void NetSslServer::get_connections( std::function<void( base::Error err, uint16_t count )> callback ) {
						Unused( callback );
						daw::exception::daw_throw_not_implemented( );
					}

					void NetSslServer::handle_handshake( NetSslServer self, NetSocketStream socket,
					                                         base::ErrorCode const &err ) {

						daw::exception::daw_throw_value_on_true( err );
						self.emitter( )->emit( "connection", socket );
					}

					void NetSslServer::handle_accept( NetSslServer self, NetSocketStream socket,
					                                      base::ErrorCode const &err ) {
						try {
							if( err.value( ) == 24 ) {
								self.emit_error( err, "Too many open files", "NetNoSslServer::handle_accept" );
							} else {
								daw::exception::daw_throw_value_on_true( err );
							  socket->socket( ).handshake_async( boost::asio::ssl::stream_base::server, [
								  socket = std::move( socket ), self = std::move( self )
							  ]( base::ErrorCode const &err1 ) mutable { handle_handshake( std::move( self ), socket, err1 ); } );
						  }
						} catch( ... ) {
							self.emit_error( std::current_exception( ), "Error while handling accept",
							                 "NetSslServer::handle_accept" );
						}
						self.start_accept( );
					}

					void NetSslServer::start_accept( ) {
						try {
							auto socket_sp = daw::nodepp::lib::net::create_net_socket_stream( m_config );
							daw::exception::daw_throw_on_false( socket_sp,
							                                    "NetSslServer::start_accept( ), Invalid socket - null" );

							socket_sp->socket( ).init( );
							auto &boost_socket = socket_sp->socket( );

							m_acceptor->async_accept( boost_socket->lowest_layer( ),
							                          [ socket_sp = std::move( socket_sp ),
								                          self = NetSslServer{*this} ]( base::ErrorCode const &err ) mutable {
								                          daw::exception::daw_throw_value_on_true( err );
								                          handle_accept( std::move( self ), socket_sp, err );
							                          } );
						} catch( ... ) {
							emit_error( std::current_exception( ), "Error while starting accept", "NetSslServer::start_accept" );
						}
					}
			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
