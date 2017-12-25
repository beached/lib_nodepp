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

#include <boost/asio/ip/tcp.hpp>
#include <list>
#include <memory>
#include <string>

#include <daw/json/daw_json_link.h>

#include "base_error.h"
#include "base_event_emitter.h"
#include "base_service_handle.h"
#include "base_types.h"
#include "lib_net_address.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				//////////////////////////////////////////////////////////////////////////
				/// @brief		A TCP Server class
				// Requires:	daw::nodepp::base::EventEmitter, daw::nodepp::base::options_t,
				//				daw::nodepp::lib::net::NetAddress, daw::nodepp::base::Error
				class NetNoSslServer : public daw::nodepp::base::StandardEvents<NetNoSslServer> {
					std::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;

				public:
					explicit NetNoSslServer( daw::nodepp::base::EventEmitter emitter );

					NetNoSslServer( ) = delete;
					~NetNoSslServer( ) override;
					NetNoSslServer( NetNoSslServer const & ) = default;
					NetNoSslServer( NetNoSslServer && ) noexcept = default;
					NetNoSslServer &operator=( NetNoSslServer const & ) = default;
					NetNoSslServer &operator=( NetNoSslServer && ) noexcept = default;

					void listen( uint16_t port, ip_version ip_ver, uint16_t max_backlog );
					void listen( uint16_t port, ip_version ip_ver );
					void listen( uint16_t port );

					void close( );

					daw::nodepp::lib::net::NetAddress address( ) const;

					void get_connections( std::function<void( daw::nodepp::base::Error err, uint16_t count )> callback );

				private:
					static void handle_handshake( NetNoSslServer obj, NetSocketStream socket, base::ErrorCode const &err );
					static void handle_accept( NetNoSslServer self, NetSocketStream socket, base::ErrorCode const &err );

					void start_accept( );
				}; // class NetNoSslServer
			}    // namespace net
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
