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

#include <daw/daw_union_pair.h>

#include "lib_net_nossl_server.h"
#include "lib_net_server.h"
#include "lib_net_ssl_server.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				/// @brief		A TCP Server class
				template<typename EventEmitter = base::StandardEventEmitter>
				class NetServer
				  : public base::BasicStandardEvents<NetServer<EventEmitter>,
				                                     EventEmitter> {

					using value_type = daw::union_pair_t<NetNoSslServer<EventEmitter>,
					                                     NetSslServer<EventEmitter>>;
					value_type m_net_server;

				public:
					explicit NetServer( EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<NetServer<EventEmitter>, EventEmitter>(
					      std::move( emitter ) )
					  , m_net_server( ) {

						m_net_server = NetNoSslServer<EventEmitter>( this->emitter( ) );
					}

					explicit NetServer( SslServerConfig const &ssl_config,
					                    EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<NetServer<EventEmitter>, EventEmitter>(
					      std::move( emitter ) )
					  , m_net_server( ) {

						m_net_server =
						  NetSslServer<EventEmitter>( ssl_config, this->emitter( ) );
					}

					bool using_ssl( ) const noexcept {
						return m_net_server.which( ) == 1;
					}

					void listen( uint16_t port, ip_version ip_ver,
					             uint16_t max_backlog ) {
						m_net_server.visit(
						  [&]( auto &Srv ) { Srv.listen( port, ip_ver, max_backlog ); } );
					}

					void listen( uint16_t port, ip_version ip_ver ) {
						m_net_server.visit(
						  [&]( auto &Srv ) { Srv.listen( port, ip_ver ); } );
					}

					void listen( uint16_t port ) {
						m_net_server.visit(
						  [&]( auto &Srv ) { Srv.listen( port, ip_version::ipv4_v6 ); } );
					}

					void close( ) {
						m_net_server.visit( [&]( auto &Srv ) { Srv.close( ); } );
					}

					NetAddress address( ) const {
						return m_net_server.visit(
						  [&]( auto const &Srv ) { return Srv.address( ); } );
					}

					template<typename Callback>
					void get_connections( Callback &&callback ) {
						static_assert(
						  daw::is_callable_v<Callback, daw::nodepp::base::Error /*err*/,
						                     uint16_t /*count*/>,
						  "Callback cannot be called with needed arguments" );

						return m_net_server.visit( [callback = std::forward<Callback>(
						                              callback )]( auto &Srv ) mutable {
							return Srv.get_connections( callback );
						} );
					}

					template<typename Listener>
					NetServer &on_connection( Listener &&listener ) {
						base::add_listener<NetSocketStream<EventEmitter>>(
						  "connection", this->emitter( ),
						  std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					NetServer &on_next_connection( Listener &&listener ) {
						base::add_listener<NetSocketStream<EventEmitter>>(
						  "connection", this->emitter( ),
						  std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					template<typename Listener>
					NetServer &on_listening( Listener &&listener ) {
						base::add_listener<EndPoint>( "listening", this->emitter( ),
						                              std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					NetServer &on_next_listening( Listener &&listener ) {
						base::add_listener<EndPoint>( "listening", this->emitter( ),
						                              std::forward<Listener>( listener ),
						                              base::callback_run_mode_t::run_once );
						return *this;
					}

					template<typename Listener>
					NetServer &on_closed( Listener &&listener ) {
						base::add_listener<>( "closed", this->emitter( ),
						                      std::forward<Listener>( listener ),
						                      base::callback_run_mode_t::run_once );
						return *this;
					}

					void emit_connection( NetSocketStream<EventEmitter> socket ) {
						this->emitter( ).emit( "connection", std::move( socket ) );
					}

					void emit_listening( EndPoint endpoint ) {
						this->emitter( ).emit( "listening", std::move( endpoint ) );
					}

					void emit_closed( ) {
						this->emitter( ).emit( "closed" );
					}
				}; // class NetServer
			}    // namespace net
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
