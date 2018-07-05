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

#include <boost/optional.hpp>
#include <cstdint>
#include <ostream>

#include <daw/daw_string_view.h>

#include "base_event_emitter.h"
#include "base_service_handle.h"
#include "lib_http_client_connection_options.h"
#include "lib_http_request.h"
#include "lib_http_server_response.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					class HttpClient;

					class HttpClientConnectionImpl;
				} // namespace impl

				class HttpClientResponseMessage {};

				using HttpClientConnection =
				  std::shared_ptr<impl::HttpClientConnectionImpl>;

				HttpClientConnection create_http_client_connection(
				  daw::nodepp::lib::net::NetSocketStream socket,
				  daw::nodepp::base::EventEmitter emitter =
				    daw::nodepp::base::EventEmitter( ) );

				/// @brief		An HTTP Client class
				class HttpClient
				  : public daw::nodepp::base::StandardEvents<HttpClient> {

					daw::nodepp::lib::net::NetSocketStream m_client;

				public:
					explicit HttpClient( daw::nodepp::base::EventEmitter &&emitter =
					                       daw::nodepp::base::EventEmitter( ) );

					void request( std::string scheme, std::string host, uint16_t port,
					              daw::nodepp::lib::http::HttpClientRequest request );

					template<typename Listener>
					HttpClient &on_connection( Listener && ) {
						static_assert(
						  daw::is_callable_v<Listener, HttpClientConnection>,
						  "Listener must take an argument of type HttpClientConnection" );

						return *this;
					}
				}; // class HttpClient

				namespace impl {
					class HttpClientConnectionImpl
					  : public daw::nodepp::base::StandardEvents<
					      HttpClientConnectionImpl> {

						daw::nodepp::lib::net::NetSocketStream m_socket;

					public:
						explicit HttpClientConnectionImpl(
						  base::EventEmitter &&emitter = base::EventEmitter( ) );

						HttpClientConnectionImpl(
						  daw::nodepp::lib::net::NetSocketStream socket,
						  daw::nodepp::base::EventEmitter &&emitter );

						template<typename Listener>
						HttpClientConnectionImpl &on_response_returned( Listener && ) {
							static_assert(
							  daw::is_callable_v<Listener,
							                     daw::nodepp::lib::http::HttpServerResponse>,
							  "Listener must take an argument of type HttpServerResponse" );

							return *this;
						}

						template<typename Listener>
						HttpClientConnectionImpl &on_next_response_returned( Listener && ) {
							static_assert(
							  daw::is_callable_v<Listener,
							                     daw::nodepp::lib::http::HttpServerResponse>,
							  "Listener must take an argument of type HttpServerResponse" );
							return *this;
						}

						template<typename Listener>
						HttpClientConnectionImpl &on_closed( Listener && ) {
							static_assert( daw::is_callable_v<Listener>,
							               "Listener must be callable without arguments" );
							// Only once as it is called on the way out
							return *this;
						}
					}; // HttpClientConnectionImpl
				}    // namespace impl
				using HttpClientConnection =
				  std::shared_ptr<impl::HttpClientConnectionImpl>;

				// TODO: should be returning a response
				template<typename Listener>
				void
				get( daw::string_view url_string,
				     std::initializer_list<
				       std::pair<std::string, HttpClientConnectionOptions::value_type>>
				       options,
				     Listener && ) {
					static_assert(
					  daw::is_callable_v<Listener, HttpClientResponseMessage>,
					  "on_completion must take an argument of type "
					  "HttpClientResponseMessage" );

					auto url = parse_url( url_string );
					std::cout << "url: " << url->to_json_string( ) << std::endl;
					std::cout << "url: " << url << std::endl;
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
