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

#pragma once

#include <boost/variant.hpp>

#include "lib_net_nossl_server.h"
#include "lib_net_ssl_server.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				/// @brief		A TCP Server class
				class NetServer : public daw::nodepp::base::StandardEvents<NetServer> {
					using value_type = boost::variant<NetNoSslServer, NetSslServer>;
					value_type m_net_server;

				public:
					explicit NetServer( daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::EventEmitter{} );

					explicit NetServer( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					                    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::EventEmitter{} );

					~NetServer( ) override;
					NetServer( NetServer const & ) = default;
					NetServer( NetServer && ) noexcept = default;
					NetServer &operator=( NetServer const & ) = default;
					NetServer &operator=( NetServer && ) noexcept = default;

					bool using_ssl( ) const noexcept;

					void listen( uint16_t port );
					void listen( uint16_t port, ip_version ip_ver );
					void listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog );

					void close( );

					daw::nodepp::lib::net::NetAddress address( ) const;

					void get_connections( std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback );

					template<typename Listener>
					NetServer &on_connection( Listener listener ) {
						emitter( )->template add_listener<NetSocketStream>( "connection", std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					NetServer &on_next_connection( Listener listener ) {
						emitter( )->template add_listener<NetSocketStream>( "connection", std::move( listener ),
						                                                    callback_runmode_t::run_once );
						return *this;
					}

					template<typename Listener>
					NetServer &on_listening( Listener listener ) {
						emitter( )->template add_listener<EndPoint>( "listening", std::move( listener ) );
						return *this;
					}

					template<typename Listener>
					NetServer &on_next_listening( Listener listener ) {
						emitter( )->template add_listener<EndPoint>( "listening", std::move( listener ),
						                                             callback_runmode_t::run_once );
						return *this;
					}

					template<typename Listener>
					NetServer &on_closed( Listener listener ) {
						emitter( )->template add_listener<>( "closed", std::move( listener ), callback_runmode_t::run_once );
						return *this;
					}

					void emit_connection( NetSocketStream socket );
					void emit_listening( EndPoint endpoint );
					void emit_closed( );
				}; // class NetServer
			}    // namespace net
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
