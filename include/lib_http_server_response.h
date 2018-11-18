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

#include <type_traits>

#include <daw/daw_string_view.h>
#include <daw/parallel/daw_observable_ptr.h>

#include "base_enoding.h"
#include "base_event_emitter.h"
#include "base_stream.h"
#include "base_types.h"
#include "lib_http.h"
#include "lib_http_headers.h"
#include "lib_http_server_response.h"
#include "lib_http_version.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					std::string gmt_timestamp( );
				}

				template<typename EventEmitter = base::StandardEventEmitter>
				class HttpServerResponse
				  : public base::stream::StreamWritableEvents<
				      HttpServerResponse<EventEmitter>>,
				    public base::BasicStandardEvents<HttpServerResponse<EventEmitter>,
				                                     EventEmitter> {

					net::NetSocketStream<EventEmitter> m_socket;
					struct response_data_t {
						HttpHeaders m_headers{};
						base::data_t m_body{};
						HttpVersion m_version = {1, 1};
						bool m_status_sent = false;
						bool m_headers_sent = false;
						bool m_body_sent = false;

						response_data_t( ) = default;
					};

					std::shared_ptr<response_data_t> m_response_data;

					template<typename Action>
					bool on_socket_if_valid( Action &&action ) {
						static_assert(
						  std::is_invocable_v<Action, net::NetSocketStream<EventEmitter> &>,
						  "Action must accept a NetSocketStream as an argument" );

						if( m_socket.expired( ) ) {
							return false;
						}
						action( m_socket );
						return true;
					}

				public:
					explicit HttpServerResponse(
					  net::NetSocketStream<EventEmitter> socket,
					  EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<HttpServerResponse<EventEmitter>,
					                              EventEmitter>( std::move( emitter ) )
					  , m_socket( std::move( socket ) )
					  , m_response_data( ) {}

					~HttpServerResponse( ) noexcept {
						// Attempt cleanup
						try {
							on_socket_if_valid( []( net::NetSocketStream<EventEmitter> &s ) {
								s.close( false );
							} );
						} catch( ... ) {
							// Do nothing
							std::cout << "HttpServerResponse: Exception";
						}
					}

					HttpServerResponse( HttpServerResponse const & ) = default;
					HttpServerResponse( HttpServerResponse && ) noexcept = default;
					HttpServerResponse &operator=( HttpServerResponse const & ) = default;
					HttpServerResponse &
					operator=( HttpServerResponse && ) noexcept = default;

					HttpServerResponse &write_raw_body( base::data_t const &data ) {
						on_socket_if_valid(
						  [&data]( net::NetSocketStream<EventEmitter> socket ) {
							  socket.write( data );
						  } );
						return *this;
					}

					template<
					  typename BytePtr,
					  std::enable_if_t<( sizeof( *std::declval<BytePtr>( ) ) == 1 ),
					                   std::nullptr_t> = nullptr>
					HttpServerResponse &write( BytePtr first, BytePtr last ) {
						m_response_data->m_body.insert( std::end( m_response_data->m_body ),
						                                first, last );
						return *this;
					}

					template<size_t N>
					HttpServerResponse &write( char const ( &buff )[N] ) {
						static_assert( N > 0, "Not sure what to do with an empty buff" );
						return write( buff, buff + ( N - 1 ) );
					}

					template<typename Container,
					         std::enable_if_t<daw::traits::is_container_like_v<Container>,
					                          std::nullptr_t> = nullptr>
					HttpServerResponse &write( Container &&container ) {
						static_assert( sizeof( *std::cbegin( container ) ),
						               "Data in container must be byte sized" );
						return this->write( std::begin( container ),
						                    std::end( container ) );
					}

					HttpServerResponse &end( ) {
						send( );
						on_socket_if_valid(
						  []( net::NetSocketStream<EventEmitter> socket ) {
							  socket.end( );
						  } );
						return *this;
					}

					template<typename... Args, std::enable_if_t<( sizeof...( Args ) > 0 ),
					                                            std::nullptr_t> = nullptr>
					HttpServerResponse &end( Args &&... args ) {
						this->write( std::forward<Args>( args )... );
						return this->end( );
					}

					void close( bool send_response = true ) {
						if( send_response ) {
							send( );
						}
						on_socket_if_valid(
						  []( net::NetSocketStream<EventEmitter> socket ) {
							  socket.end( ).close( );
						  } );
					}

					void start( ) noexcept {
						try {
							HttpServerResponse self( *this );
							on_socket_if_valid(
							  [&]( net::NetSocketStream<EventEmitter> socket ) {
								  socket.on_write_completion(
								    [self = mutable_capture( self )]( auto ) {
									    self->emit_write_completion( *self );
								    } );
								  socket.on_all_writes_completed(
								    [self = mutable_capture( self )]( auto ) {
									    self->emit_all_writes_completed( *self );
								    } );
							  } );
						} catch( ... ) {}
					}

					HttpHeaders &headers( ) {
						return m_response_data->m_headers;
					}

					HttpHeaders const &headers( ) const {
						return m_response_data->m_headers;
					}

					base::data_t const &body( ) const {
						return m_response_data->m_body;
					}

					HttpServerResponse &send_status( uint16_t status_code = 200 ) {
						auto status = HttpStatusCodes( status_code );
						std::string msg =
						  "HTTP/" + m_response_data->m_version.to_string( ) + " " +
						  std::to_string( status.first ) + " " + status.second + "\r\n";

						m_response_data->m_status_sent = on_socket_if_valid(
						  [&msg]( net::NetSocketStream<EventEmitter> socket ) {
							  socket.write_async( msg ); // TODO: make faster
						  } );
						return *this;
					}

					HttpServerResponse &send_status( uint16_t status_code,
					                                 daw::string_view status_msg ) {
						std::string msg = "HTTP/" +
						                  m_response_data->m_version.to_string( ) + " " +
						                  std::to_string( status_code ) + " " +
						                  status_msg.to_string( ) + "\r\n";

						m_response_data->m_status_sent = on_socket_if_valid(
						  [&msg]( net::NetSocketStream<EventEmitter> socket ) {
							  socket.write_async( msg ); // TODO: make faster
						  } );
						return *this;
					}

					HttpServerResponse &send_headers( ) {
						m_response_data->m_headers_sent = on_socket_if_valid(
						  [&]( net::NetSocketStream<EventEmitter> socket ) {
							  auto &dte = m_response_data->m_headers["Date"];
							  if( dte.empty( ) ) {
								  dte = impl::gmt_timestamp( );
							  }
							  socket.write_async( m_response_data->m_headers.to_string( ) );
						  } );
						return *this;
					}

					HttpServerResponse &send_body( ) {
						m_response_data->m_body_sent = on_socket_if_valid(
						  [&]( net::NetSocketStream<EventEmitter> socket ) {
							  HttpHeader content_header{
							    "Content-Length",
							    std::to_string( m_response_data->m_body.size( ) )};
							  socket.write_async( content_header.to_string( ) );
							  socket.write_async( "\r\n\r\n" );
							  socket.write_async( m_response_data->m_body );
						  } );
						return *this;
					}

					HttpServerResponse &clear_body( ) {
						m_response_data->m_body.clear( );
						return *this;
					}

					bool send( ) {
						bool result = false;
						if( !m_response_data->m_status_sent ) {
							result = true;
							send_status( );
						}
						if( !m_response_data->m_headers_sent ) {
							result = true;
							send_headers( );
						}
						if( !m_response_data->m_body_sent ) {
							result = true;
							send_body( );
						}
						return result;
					}
					HttpServerResponse &reset( ) {
						m_response_data->m_status_sent = false;
						m_response_data->m_headers.headers.clear( );
						m_response_data->m_headers_sent = false;
						clear_body( );
						m_response_data->m_body_sent = false;
						return *this;
					}

					bool is_open( ) {
						return !m_socket.expired( ) and m_socket.is_open( );
					}

					bool is_closed( ) const {
						return m_socket.expired( ) or m_socket.is_closed( );
					}

					bool can_write( ) const {
						return !m_socket.expired( ) and m_socket.can_write( );
					}

					HttpServerResponse &add_header( daw::string_view header_name,
					                                daw::string_view header_value ) {
						m_response_data->m_headers.add( header_name.to_string( ),
						                                header_value.to_string( ) );
						return *this;
					}

					HttpServerResponse &prepare_raw_write( size_t content_length ) {
						on_socket_if_valid(
						  [&]( net::NetSocketStream<EventEmitter> socket ) {
							  m_response_data->m_body_sent = true;
							  m_response_data->m_body.clear( );
							  send( );
							  HttpHeader content_header{"Content-Length",
							                            std::to_string( content_length )};
							  socket.write_async( content_header.to_string( ) );
							  socket.write_async( "\r\n\r\n" );
						  } );
						return *this;
					}

					HttpServerResponse &write_file( daw::string_view file_name ) {
						on_socket_if_valid(
						  [file_name]( net::NetSocketStream<EventEmitter> socket ) {
							  socket.send_file( file_name );
						  } );
						return *this;
					}

					HttpServerResponse &write_file_async( string_view file_name ) {
						on_socket_if_valid(
						  [file_name]( net::NetSocketStream<EventEmitter> socket ) {
							  socket.send_file_async( file_name );
						  } );
						return *this;
					}

				}; // struct HttpServerResponse

				template<typename EventEmitter>
				void create_http_server_error_response(
				  HttpServerResponse<EventEmitter> response, uint16_t error_no ) {
					auto msg = HttpStatusCodes( error_no );
					if( msg.first != error_no ) {
						msg.first = error_no;
						msg.second = "Error";
					}
					std::string end_msg =
					  std::to_string( msg.first ) + " " + msg.second + "\r\n";
					response.send_status( msg.first, msg.second )
					  .add_header( "Content-Type", "text/plain" )
					  .add_header( "Connection", "close" )
					  .end( end_msg )
					  .close( );
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
