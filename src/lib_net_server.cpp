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

#include "lib_net_server.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using namespace daw::nodepp;
				using namespace boost::asio::ip;
				/// @brief		A TCP Server class
				///
				NetServer::NetServer( daw::nodepp::base::EventEmitter emitter )
				  : StandardEvents<NetServer>{emitter}
				  , m_net_server{NetNoSslServer{emitter}} {}

				NetServer::NetServer( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				                              daw::nodepp::base::EventEmitter emitter )
				  : StandardEvents<NetServer>{emitter}
				  , m_net_server{NetSslServer{ssl_config, emitter}} {}

				NetServer::~NetServer( ) = default;

				bool NetServer::using_ssl( ) const noexcept {
					return m_net_server.which( ) == 1;
				}

				void NetServer::listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog ) {
					boost::apply_visitor( [port, max_backlog, ip_ver]( auto &Srv ) { Srv.listen( port, ip_ver, max_backlog ); },
					                      m_net_server );
				}

				void NetServer::listen( uint16_t port, ip_version ip_ver ) {
					boost::apply_visitor( [port, ip_ver]( auto &Srv ) { Srv.listen( port, ip_ver ); }, m_net_server );
				}

				void NetServer::listen( uint16_t port ) {
					boost::apply_visitor( [port]( auto &Srv ) { Srv.listen( port, ip_version::ipv4_v6 ); }, m_net_server );
				}

				void NetServer::close( ) {
					boost::apply_visitor( []( auto &Srv ) { Srv.close( ); }, m_net_server );
				}

				NetAddress NetServer::address( ) const {
					return boost::apply_visitor( []( auto &Srv ) -> NetAddress { return Srv.address( ); }, m_net_server );
				}

				void
				NetServer::get_connections( std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback ) {

					boost::apply_visitor( [&callback]( auto &Srv ) { Srv.get_connections( callback ); }, m_net_server );
				}

				void NetServer::emit_connection( NetSocketStream socket ) {
					emitter( )->emit( "connection", std::move( socket ) );
				}

				void NetServer::emit_listening( EndPoint endpoint ) {
					emitter( )->emit( "listening", std::move( endpoint ) );
				}

				void NetServer::emit_closed( ) {
					emitter( )->emit( "closed" );
				}
			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
