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

#include <cstdint>
#include <ostream>
#include <type_traits>

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
					template<typename EventEmitter = base::StandardEventEmitter>
					class HttpClient;

					template<typename EventEmitter = base::StandardEventEmitter>
					class HttpClientConnectionImpl;
				} // namespace impl

				class HttpClientResponseMessage {};

				template<typename EventEmitter = base::StandardEventEmitter>
				using HttpClientConnection =
				  std::shared_ptr<impl::HttpClientConnectionImpl<EventEmitter>>;

				template<typename EventEmitter = base::StandardEventEmitter>
				HttpClientConnection<EventEmitter> create_http_client_connection(
				  net::NetSocketStream<EventEmitter> socket,
				  EventEmitter emitter = EventEmitter( ) ) {

					return std::make_shared<impl::HttpClientConnectionImpl<EventEmitter>>(
					  std::move( socket ), std::move( emitter ) );
				}

				/// @brief		An HTTP Client class
				template<typename EventEmitter>
				class HttpClient
				  : public base::BasicStandardEvents<HttpClient<EventEmitter>,
				                                     EventEmitter> {

					net::NetSocketStream<EventEmitter> m_client;

				public:
					explicit HttpClient( EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<HttpClient, EventEmitter>(
					      std::move( emitter ) )
					  , m_client( ) {}

					void request( std::string scheme, std::string host, uint16_t port,
					              HttpClientRequest request ) {
						m_client
						  .on_connected(
						    [scheme = std::move( scheme ), host = std::move( host ), port,
						     request = mutable_capture( std::move( request ) )]( auto s ) {
							    auto const &request_line = request->request_line;
							    auto ss = std::stringstream( );
							    ss << to_string( request_line.method ) << " "
							       << to_string( request_line.url ) << " HTTP/1.1\r\n";
							    ss << "Host: " << host << ":" << std::to_string( port )
							       << "\r\n\r\n";
							    auto msg = ss.str( );
							    s.end( msg );
							    s.set_read_mode(
							      net::NetSocketStreamReadMode::double_newline );
							    s.read_async( );
						    } )
						  .on_data_received( []( base::shared_data_t data_buffer, bool ) {
							  if( data_buffer ) {
								  for( auto const &ch : *data_buffer ) {
									  std::cout << ch;
								  }
								  std::cout << std::endl;
							  }
						  } );

						m_client.connect( host, port );
					}

					template<typename Listener>
					HttpClient &on_connection( Listener && ) {
						static_assert(
						  std::is_invocable_v<Listener, HttpClientConnection>,
						  "Listener must take an argument of type HttpClientConnection" );

						return *this;
					}
				}; // class HttpClient

				namespace impl {
					template<typename EventEmitter>
					class HttpClientConnectionImpl
					  : public base::BasicStandardEvents<
					      HttpClientConnectionImpl<EventEmitter>, EventEmitter> {

						net::NetSocketStream<EventEmitter> m_socket;

					public:
						explicit HttpClientConnectionImpl(
						  EventEmitter &&emitter = EventEmitter( ) )
						  : base::BasicStandardEvents( std::move( emitter ) ) {}

						HttpClientConnectionImpl( net::NetSocketStream<EventEmitter> socket,
						                          EventEmitter &&emitter )
						  : base::BasicStandardEvents<HttpClientConnectionImpl,
						                              EventEmitter>( std::move( emitter ) )
						  , m_socket( std::move( socket ) ) {}

						template<typename Listener>
						HttpClientConnectionImpl &on_response_returned( Listener && ) {
							static_assert(
							  std::is_invocable_v<Listener,
							                      daw::nodepp::lib::http::HttpServerResponse>,
							  "Listener must take an argument of type HttpServerResponse" );

							return *this;
						}

						template<typename Listener>
						HttpClientConnectionImpl &on_next_response_returned( Listener && ) {
							static_assert(
							  std::is_invocable_v<Listener,
							                      daw::nodepp::lib::http::HttpServerResponse>,
							  "Listener must take an argument of type HttpServerResponse" );
							return *this;
						}

						template<typename Listener>
						HttpClientConnectionImpl &on_closed( Listener && ) {
							static_assert( std::is_invocable_v<Listener>,
							               "Listener must be callable without arguments" );
							// Only once as it is called on the way out
							return *this;
						}
					}; // HttpClientConnectionImpl
				}    // namespace impl
				template<typename EventEmitter = base::StandardEventEmitter>
				using HttpClientConnection =
				  std::shared_ptr<impl::HttpClientConnectionImpl<EventEmitter>>;

				// TODO: should be returning a response
				template<typename Listener>
				void
				get( daw::string_view url_string,
				     std::initializer_list<
				       std::pair<std::string, HttpClientConnectionOptions::value_type>>
				       options,
				     Listener && ) {
					static_assert(
					  std::is_invocable_v<Listener, HttpClientResponseMessage>,
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
