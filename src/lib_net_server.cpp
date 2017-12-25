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
				namespace impl {
					using namespace daw::nodepp;
					using namespace boost::asio::ip;
					/// @brief		A TCP Server class
					///
					NetServerImpl::NetServerImpl( daw::nodepp::base::EventEmitter emitter )
					  : StandardEvents<NetServerImpl>{emitter}
					  , m_net_server{daw::nodepp::impl::make_shared_ptr<NetNoSslServerImpl>( emitter )} {}

					NetServerImpl::NetServerImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					                              daw::nodepp::base::EventEmitter emitter )
					  : StandardEvents<NetServerImpl>{emitter}
					  , m_net_server{daw::nodepp::impl::make_shared_ptr<NetSslServerImpl>( ssl_config, emitter )} {}

					NetServerImpl::~NetServerImpl( ) = default;

					bool NetServerImpl::using_ssl( ) const noexcept {
						return m_net_server.which( ) == 1;
					}

					void NetServerImpl::listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog ) {
						boost::apply_visitor(
						  [port, max_backlog, ip_ver]( auto &Srv ) { Srv->listen( port, ip_ver, max_backlog ); }, m_net_server );
					}

					void NetServerImpl::listen( uint16_t port, ip_version ip_ver ) {
						boost::apply_visitor( [port, ip_ver]( auto &Srv ) { Srv->listen( port, ip_ver ); }, m_net_server );
					}

					void NetServerImpl::listen( uint16_t port ) {
						boost::apply_visitor( [port]( auto &Srv ) { Srv->listen( port, ip_version::ipv4_v6 ); }, m_net_server );
					}

					void NetServerImpl::close( ) {
						boost::apply_visitor( []( auto &Srv ) { Srv->close( ); }, m_net_server );
					}

					NetAddress NetServerImpl::address( ) const {
						return boost::apply_visitor( []( auto &Srv ) -> NetAddress { return Srv->address( ); }, m_net_server );
					}

					void NetServerImpl::get_connections(
					  std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback ) {

						boost::apply_visitor( [&callback]( auto &Srv ) { Srv->get_connections( callback ); }, m_net_server );
					}

					void NetServerImpl::emit_connection( NetSocketStream socket ) {
						emitter( )->emit( "connection", std::move( socket ) );
					}

					void NetServerImpl::emit_listening( EndPoint endpoint ) {
						emitter( )->emit( "listening", std::move( endpoint ) );
					}

					void NetServerImpl::emit_closed( ) {
						emitter( )->emit( "closed" );
					}
				} // namespace impl

				NetServer::NetServer( base::EventEmitter emitter )
				  : m_net_server{daw::nodepp::impl::make_shared_ptr<impl::NetServerImpl>( std::move( emitter ) )} {}

				NetServer::NetServer( daw::nodepp::lib::net::SslServerConfig const &ssl_config, base::EventEmitter emitter )
				  : m_net_server{daw::nodepp::impl::make_shared_ptr<impl::NetServerImpl>( ssl_config, std::move( emitter ) )} {}

				bool NetServer::using_ssl( ) const noexcept {
					return m_net_server->using_ssl( );
				}

				void NetServer::emit_closed( ) {
					m_net_server->emit_closed( );
				}

				void NetServer::emit_listening( EndPoint endpoint ) {
					m_net_server->emit_listening( std::move( endpoint ) );
				}

				void NetServer::emit_connection( NetSocketStream socket ) {
					m_net_server->emit_connection( std::move( socket ) );
				}

				void
				NetServer::get_connections( std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback ) {
					m_net_server->get_connections( std::move( callback ) );
				}

				daw::nodepp::lib::net::NetAddress NetServer::address( ) const {
					return m_net_server->address( );
				}

				void NetServer::close( ) {
					m_net_server->close( );
				}

				void NetServer::listen( uint16_t port ) {
					m_net_server->listen( port );
				}

				void NetServer::listen( uint16_t port, ip_version ip_ver ) {
					m_net_server->listen( port, ip_ver );
				}

				void NetServer::listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog ) {
					m_net_server->listen( port, ip_ver, max_backlog );
				}

				base::EventEmitter &NetServer::emitter( ) {
					return m_net_server->emitter( );
				}

				base::EventEmitter const &NetServer::emitter( ) const {
					return m_net_server->emitter( );
				}

				void NetServer::emit_error( std::string description, std::string where ) {
					m_net_server->emit_error( std::move( description ), std::move( where ) );
				}

				void NetServer::emit_error( base::Error const &child, std::string description, std::string where ) {
					m_net_server->emit_error( child, std::move( description ), std::move( where ) );
				}

				void NetServer::emit_error( base::ErrorCode const &error, std::string description, std::string where ) {
					m_net_server->emit_error( error, std::move( description ), std::move( where ) );
				}

				void NetServer::emit_error( std::exception_ptr ex, std::string description, std::string where ) {
					m_net_server->emit_error( std::move( ex ), std::move( description ), std::move( where ) );
				}

				void NetServer::emit_exit( base::Error error ) {
					m_net_server->emit_exit( std::move( error ) );
				}

				void NetServer::emit_exit( ) {
					m_net_server->emit_exit( );
				}

				impl::NetServerImpl const &NetServer::base( ) const {
					return *m_net_server;
				}

				impl::NetServerImpl &NetServer::base( ) {
					return *m_net_server;
				}

				NetServer::operator bool( ) const noexcept {
					return static_cast<bool>( m_net_server );
				}

				NetServer & NetServer::on_connection( std::function<void( NetSocketStream )> listener ) {
					m_net_server->on_connection( std::move( listener ) );
					return *this;
				}

				NetServer & NetServer::on_next_connection( std::function<void( NetSocketStream )> listener ) {
					m_net_server->on_next_connection( std::move( listener ) );
					return *this;
				}

				NetServer & NetServer::on_closed( std::function<void( )> listener ) {
					m_net_server->on_closed( std::move( listener ) );
					return *this;
				}

				NetServer & NetServer::on_listening( std::function<void( EndPoint )> listener ) {
					m_net_server->on_listening( std::move( listener ) );
					return *this;
				}

				NetServer & NetServer::on_next_listening( std::function<void( EndPoint )> listener ) {
					m_net_server->on_next_listening( std::move( listener ) );
					return *this;
				}

			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
