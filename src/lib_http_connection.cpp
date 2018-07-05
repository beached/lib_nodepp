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

#include <boost/regex.hpp>
#include <memory>
#include <regex>

#include "lib_http_connection.h"
#include "lib_http_request.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				HttpServerConnection::HttpServerConnection(
				  lib::net::NetSocketStream &&socket, base::EventEmitter &&emitter )
				  : daw::nodepp::base::StandardEvents<HttpServerConnection>(
				      std::move( emitter ) )
				  , m_socket( std::move( socket ) ) {}

				void HttpServerConnection::start( ) {

					m_socket
					  .on_next_data_received(
					    [obj = *this]( std::shared_ptr<base::data_t> data_buffer,
					                   bool ) mutable {
						    daw::exception::daw_throw_on_false(
						      data_buffer,
						      "Null buffer passed to NetSocketStream->on_data_received "
						      "event" );

						    try {
							    HttpServerResponse response{obj.m_socket};
							    response.start( );
							    try {
								    auto request = parse_http_request( daw::string_view{
								      data_buffer->data( ), data_buffer->size( )} );
								    data_buffer.reset( );
								    obj.emit_request_made( std::move( request ),
								                           std::move( response ) );
							    } catch( ... ) {
								    create_http_server_error_response( std::move( response ),
								                                       400 );
								    obj.emit_error(
								      std::current_exception( ), "Error parsing http request",
								      "HttpServerConnection::start#on_next_data_received#3" );
							    }
						    } catch( ... ) {
							    obj.emit_error(
							      std::current_exception( ),
							      "Exception in processing received data",
							      "HttpConnectionImpl::start#on_next_data_received" );
						    }
					    } )
					  .delegate_to<>( "closed", emitter( ), "closed" )
					  .on_error( emitter( ), "Socket Error", "HttpConnectionImpl::start" )
					  .set_read_mode( lib::net::NetSocketStreamReadMode::double_newline );

					m_socket.read_async( );
				}

				void HttpServerConnection::close( ) {
					m_socket.close( );
				}

				void HttpServerConnection::emit_closed( ) {
					emitter( ).emit( "closed" );
				}

				void HttpServerConnection::emit_client_error( base::Error error ) {
					emitter( ).emit( "client_error", error );
				}

				void
				HttpServerConnection::emit_request_made( HttpClientRequest request,
				                                         HttpServerResponse response ) {
					emitter( ).emit( "request_made", request, response );
				}

				lib::net::NetSocketStream HttpServerConnection::socket( ) {
					return m_socket;
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
