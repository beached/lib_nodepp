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

#include <memory>

#include "lib_http_request.h"
#include "lib_http_server_response.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				enum class HttpConnectionState : uint_fast8_t { Request, Message };

				template<typename EventEmitter>
				class basic_http_server_connection_t
				  : public base::BasicStandardEvents<
				      basic_http_server_connection_t<EventEmitter>, EventEmitter> {

					using base::BasicStandardEvents<
					  basic_http_server_connection_t<EventEmitter>,
					  EventEmitter>::emitter;

					net::NetSocketStream<EventEmitter> m_socket;

				public:
					explicit basic_http_server_connection_t(
					  net::NetSocketStream<EventEmitter> &&socket,
					  EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<
					      basic_http_server_connection_t<EventEmitter>, EventEmitter>(
					      daw::move( emitter ) )
					  , m_socket( daw::move( socket ) ) {}

					// Event callbacks
					template<typename Listener>
					basic_http_server_connection_t &
					on_client_error( Listener &&listener ) {
						base::add_listener<base::Error>(
						  "client_error", emitter( ), std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					basic_http_server_connection_t &
					on_next_client_error( Listener &&listener ) {
						base::add_listener<base::Error>(
						  "client_error", emitter( ), std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					template<typename Listener>
					basic_http_server_connection_t &
					on_request_made( Listener &&listener ) {
						base::add_listener<HttpClientRequest,
						                   HttpServerResponse<EventEmitter> &>(
						  "request_made", emitter( ), std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					basic_http_server_connection_t &
					on_next_request_made( Listener &&listener ) {
						base::add_listener<HttpClientRequest,
						                   HttpServerResponse<EventEmitter>>(
						  "request_made", emitter( ), std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when the connection is closed
					template<typename Listener>
					basic_http_server_connection_t &on_closed( Listener &&listener ) {
						base::add_listener( "closed", emitter( ),
						                      std::forward<Listener>( listener ),
						                      base::callback_run_mode_t::run_once );
						return *this;
					}

					void close( ) {
						m_socket.close( );
					}

					void start( ) {
						m_socket
						  .on_next_data_received(
						    [obj = mutable_capture( *this )]( auto &&data_buffer, bool ) {
							    // TODO should this be inside try block
							    daw::exception::precondition_check(
							      data_buffer,
							      "Null buffer passed to NetSocketStream->on_data_received "
							      "event" );

							    try {
								    auto response =
								      HttpServerResponse<EventEmitter>( obj->m_socket );
								    response.start( );
								    try {
									    auto request = parse_http_request( daw::string_view(
									      data_buffer->data( ), data_buffer->size( ) ) );

									    // TODO, determine if now or scope end
									    data_buffer.reset( );

									    obj->emit_request_made( daw::move( request ),
									                            daw::move( response ) );
								    } catch( ... ) {
									    create_http_server_error_response( daw::move( response ),
									                                       400 );
									    obj->emit_error( std::current_exception( ),
									                     "Error parsing http request",
									                     "start#on_next_data_received#3" );
								    }
							    } catch( ... ) {
								    obj->emit_error(
								      std::current_exception( ),
								      "Exception in processing received data",
								      "HttpConnectionImpl::start#on_next_data_received" );
							    }
						    } )
						  .template delegate_to<>( "closed", emitter( ), "closed" )
						  .on_error( emitter( ), "Socket Error",
						             "HttpConnectionImpl::start" )
						  .set_read_mode( net::NetSocketStreamReadMode::double_newline );

						m_socket.read_async( );
					}

					net::NetSocketStream<EventEmitter> socket( ) {
						return m_socket;
					}

					void emit_closed( ) {
						emitter( ).emit( "closed" );
					}

					void emit_client_error( base::Error error ) {
						emitter( ).emit( "client_error", error );
					}

					void emit_request_made( HttpClientRequest request,
					                        HttpServerResponse<EventEmitter> response ) {
						emitter( ).emit( "request_made", request, response );
					}
				};

				using HttpServerConnection =
				  basic_http_server_connection_t<base::StandardEventEmitter>;
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
