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

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdint>
#include <date/date.h>
#include <string>

#include <daw/daw_range_algorithm.h>
#include <daw/daw_string.h>
#include <daw/daw_string_view.h>
#include <daw/daw_utility.h>

#include "base_enoding.h"
#include "base_stream.h"
#include "lib_http.h"
#include "lib_http_headers.h"
#include "lib_http_server_response.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				using namespace daw::nodepp;
				HttpServerResponse::HttpServerResponse( lib::net::NetSocketStream socket, base::EventEmitter emitter )
				  : daw::nodepp::base::StandardEvents<HttpServerResponse>{std::move( emitter )}
				  , m_socket{std::move( socket )}
				  , m_response_data( ) {}

				HttpServerResponse::response_data_t::response_data_t( )
				  : m_version{1, 1}
				  , m_status_sent{false}
				  , m_headers_sent{false}
				  , m_body_sent{false} {}

				void HttpServerResponse::start( ) {
					try {
						HttpServerResponse self{*this};
						on_socket_if_valid( [&]( lib::net::NetSocketStream socket ) {
							socket.on_write_completion( [self]( auto ) mutable {
								self.emit_write_completion( self );
							} );
							socket.on_all_writes_completed( [self]( auto ) mutable {
								self.emit_all_writes_completed( self );
							} );
						} );
					} catch( ... ) {}
				}

				HttpServerResponse &HttpServerResponse::write_raw_body( base::data_t const &data ) {
					on_socket_if_valid( [&data]( lib::net::NetSocketStream socket ) { socket.write( data ); } );
					return *this;
				}

				HttpServerResponse &HttpServerResponse::write_file( daw::string_view file_name ) {
					on_socket_if_valid( [file_name]( lib::net::NetSocketStream socket ) { socket.send_file( file_name ); } );
					return *this;
				}

				HttpServerResponse &HttpServerResponse::write_file_async( string_view file_name ) {
					on_socket_if_valid(
					  [file_name]( lib::net::NetSocketStream socket ) { socket.send_file_async( file_name ); } );
					return *this;
				}

				HttpServerResponse &HttpServerResponse::clear_body( ) {
					m_response_data->m_body.clear( );
					return *this;
				}

				HttpHeaders &HttpServerResponse::headers( ) {
					return m_response_data->m_headers;
				}

				HttpHeaders const &HttpServerResponse::headers( ) const {
					return m_response_data->m_headers;
				}

				daw::nodepp::base::data_t const &HttpServerResponse::body( ) const {
					return m_response_data->m_body;
				}

				HttpServerResponse &HttpServerResponse::send_status( uint16_t status_code ) {
					auto status = HttpStatusCodes( status_code );
					std::string msg = "HTTP/" + m_response_data->m_version.to_string( ) + " " + std::to_string( status.first ) +
					                  " " + status.second + "\r\n";

					m_response_data->m_status_sent = on_socket_if_valid( [&msg]( lib::net::NetSocketStream socket ) {
						socket.write_async( msg ); // TODO: make faster
					} );
					return *this;
				}

				HttpServerResponse &HttpServerResponse::send_status( uint16_t status_code, daw::string_view status_msg ) {
					std::string msg = "HTTP/" + m_response_data->m_version.to_string( ) + " " + std::to_string( status_code ) +
					                  " " + status_msg.to_string( ) + "\r\n";

					m_response_data->m_status_sent = on_socket_if_valid( [&msg]( lib::net::NetSocketStream socket ) {
						socket.write_async( msg ); // TODO: make faster
					} );
					return *this;
				}

				namespace {
					std::string gmt_timestamp( ) {
						return date::format( "%a, %d %b %Y %H:%M:%S GMT",
						                     date::floor<std::chrono::seconds>( std::chrono::system_clock::now( ) ) );
					}
				} // namespace

				HttpServerResponse &HttpServerResponse::send_headers( ) {
					m_response_data->m_headers_sent = on_socket_if_valid( [&]( lib::net::NetSocketStream socket ) {
						auto &dte = m_response_data->m_headers["Date"];
						if( dte.empty( ) ) {
							dte = gmt_timestamp( );
						}
						socket.write_async( m_response_data->m_headers.to_string( ) );
					} );
					return *this;
				}

				HttpServerResponse &HttpServerResponse::send_body( ) {
					m_response_data->m_body_sent = on_socket_if_valid( [&]( lib::net::NetSocketStream socket ) {
						HttpHeader content_header{"Content-Length", std::to_string( m_response_data->m_body.size( ) )};
						socket.write_async( content_header.to_string( ) );
						socket.write_async( "\r\n\r\n" );
						socket.write_async( m_response_data->m_body );
					} );
					return *this;
				}

				HttpServerResponse &HttpServerResponse::prepare_raw_write( size_t content_length ) {
					on_socket_if_valid( [&]( lib::net::NetSocketStream socket ) {
						m_response_data->m_body_sent = true;
						m_response_data->m_body.clear( );
						send( );
						HttpHeader content_header{"Content-Length", std::to_string( content_length )};
						socket.write_async( content_header.to_string( ) );
						socket.write_async( "\r\n\r\n" );
					} );
					return *this;
				}

				bool HttpServerResponse::send( ) {
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

				HttpServerResponse &HttpServerResponse::end( ) {
					send( );
					on_socket_if_valid( []( lib::net::NetSocketStream socket ) { socket.end( ); } );
					return *this;
				}

				void HttpServerResponse::close( bool send_response ) {
					if( send_response ) {
						send( );
					}
					on_socket_if_valid( []( lib::net::NetSocketStream socket ) { socket.end( ).close( ); } );
				}

				HttpServerResponse &HttpServerResponse::reset( ) {
					m_response_data->m_status_sent = false;
					m_response_data->m_headers.headers.clear( );
					m_response_data->m_headers_sent = false;
					clear_body( );
					m_response_data->m_body_sent = false;
					return *this;
				}

				bool HttpServerResponse::is_closed( ) const {
					return m_socket.expired( ) || m_socket.is_closed( );
				}

				bool HttpServerResponse::can_write( ) const {
					return !m_socket.expired( ) && m_socket.can_write( );
				}

				bool HttpServerResponse::is_open( ) {
					return !m_socket.expired( ) && m_socket.is_open( );
				}

				HttpServerResponse &HttpServerResponse::add_header( daw::string_view header_name,
				                                                    daw::string_view header_value ) {
					m_response_data->m_headers.add( header_name.to_string( ), header_value.to_string( ) );
					return *this;
				}

				HttpServerResponse::~HttpServerResponse( ) {
					// Attempt cleanup
					try {
						on_socket_if_valid( []( net::NetSocketStream & s ) { s.close( false ); } );
					} catch( ... ) {
						// Do nothing
						std::cout << "HttpServerResponse: Exception";
					}
				}

				void create_http_server_error_response( HttpServerResponse response, uint16_t error_no ) {
					auto msg = HttpStatusCodes( error_no );
					if( msg.first != error_no ) {
						msg.first = error_no;
						msg.second = "Error";
					}
					std::string end_msg = std::to_string( msg.first ) + " " + msg.second + "\r\n";
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
