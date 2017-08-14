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
					//////////////////////////////////////////////////////////////////////////
					// Summary:		A TCP Server class
					// Requires:	daw::nodepp::base::EventEmitter, daw::nodepp::base::options_t,
					//				daw::nodepp::lib::net::NetAddress, daw::nodepp::base::Error
					NetServerImpl::NetServerImpl( daw::nodepp::base::EventEmitter emitter )
					    : StandardEvents<NetServerImpl>{emitter}
					    , m_net_server{std::make_shared<NetNoSslServerImpl>( emitter )} {}

					NetServerImpl::NetServerImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					                              daw::nodepp::base::EventEmitter emitter )
					    : StandardEvents<NetServerImpl>{emitter}
					    , m_net_server{std::make_shared<NetSslServerImpl>( ssl_config, emitter )} {}

					NetServerImpl::~NetServerImpl( ) = default;

					bool NetServerImpl::using_ssl( ) const noexcept {
						return m_net_server.which( ) == 1;
					}

					void NetServerImpl::listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog ) {
						boost::apply_visitor(
						    [port, max_backlog, ip_ver]( auto &Srv ) { Srv->listen( port, ip_ver, max_backlog ); },
						    m_net_server );
					}

					void NetServerImpl::close( ) {
						boost::apply_visitor( []( auto &Srv ) { Srv->close( ); }, m_net_server );
					}

					daw::nodepp::lib::net::NetAddress const &NetServerImpl::address( ) const {
						return boost::apply_visitor(
						    []( auto &Srv ) -> daw::nodepp::lib::net::NetAddress const & { return Srv->address( ); },
						    m_net_server );
					}

					void NetServerImpl::get_connections(
					    std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback ) {

						boost::apply_visitor( [&callback]( auto &Srv ) { Srv->get_connections( callback ); },
						                      m_net_server );
					}

					// Event callbacks
					NetServerImpl &
					NetServerImpl::on_connection( std::function<void( NetSocketStream socket )> listener ) {
						emitter( )->add_listener( "connection", std::move( listener ) );
						return *this;
					}

					NetServerImpl &
					NetServerImpl::on_next_connection( std::function<void( NetSocketStream socket )> listener ) {
						emitter( )->add_listener( "connection", std::move( listener ), true );
						return *this;
					}

					NetServerImpl &NetServerImpl::on_listening( std::function<void( EndPoint )> listener ) {
						emitter( )->add_listener( "listening", std::move( listener ) );
						return *this;
					}

					NetServerImpl &NetServerImpl::on_next_listening( std::function<void( )> listener ) {
						emitter( )->add_listener( "listening", std::move( listener ), true );
						return *this;
					}

					NetServerImpl &NetServerImpl::on_closed( std::function<void( )> listener ) {
						emitter( )->add_listener( "closed", std::move( listener ), true );
						return *this;
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

				NetServer create_net_server( daw::nodepp::base::EventEmitter emitter ) {
					auto result = new impl::NetServerImpl{std::move( emitter )};
					return NetServer{result};
				}

				NetServer create_net_server( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				                             daw::nodepp::base::EventEmitter emitter ) {

					auto result = new impl::NetServerImpl{ssl_config, std::move( emitter )};
					return NetServer{result};
				}
			} // namespace net
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
