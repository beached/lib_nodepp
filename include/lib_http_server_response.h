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

#include <daw/daw_string_view.h>

#include "base_enoding.h"
#include "base_event_emitter.h"
#include "base_stream.h"
#include "base_types.h"
#include "lib_http_headers.h"
#include "lib_http_version.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					class HttpServerResponseImpl;
				}
				using HttpServerResponse = std::shared_ptr<impl::HttpServerResponseImpl>;

				HttpServerResponse create_http_server_response(
				  std::weak_ptr<daw::nodepp::lib::net::impl::NetSocketStreamImpl> socket,
				  daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

				void create_http_server_error_response( HttpServerResponse const &response, uint16_t error_no );

				namespace impl {
					class HttpServerResponseImpl : public daw::nodepp::base::enable_shared<HttpServerResponseImpl>,
					                               public daw::nodepp::base::stream::StreamWritableEvents<HttpServerResponseImpl>,
					                               public daw::nodepp::base::StandardEvents<HttpServerResponseImpl> {
						std::weak_ptr<daw::nodepp::lib::net::impl::NetSocketStreamImpl> m_socket;
						HttpHeaders m_headers;
						daw::nodepp::base::data_t m_body;
						HttpVersion m_version;
						bool m_status_sent;
						bool m_headers_sent;
						bool m_body_sent;

						HttpServerResponseImpl( std::weak_ptr<daw::nodepp::lib::net::impl::NetSocketStreamImpl> socket,
						                        daw::nodepp::base::EventEmitter emitter );

						// std::function<void( lib::net::NetSocketStream )>
						template<typename Action>
						bool on_socket_if_valid( Action action ) {
							if( m_socket.expired( ) ) {
								return false;
							}
							action( m_socket.lock( ) );
							return true;
						}

					public:
						static std::shared_ptr<HttpServerResponseImpl>
						  create( std::weak_ptr<daw::nodepp::lib::net::impl::NetSocketStreamImpl>,
						          daw::nodepp::base::EventEmitter );

						~HttpServerResponseImpl( ) override;
						HttpServerResponseImpl( HttpServerResponseImpl const & ) = delete;
						HttpServerResponseImpl( HttpServerResponseImpl && ) noexcept = default;
						HttpServerResponseImpl &operator=( HttpServerResponseImpl const & ) = delete;
						HttpServerResponseImpl &operator=( HttpServerResponseImpl && ) noexcept = default;

						HttpServerResponseImpl &write_raw_body( base::data_t const &data );

						template<typename BytePtr,
						         std::enable_if_t<( sizeof( *std::declval<BytePtr>( ) ) == 1 ), std::nullptr_t> = nullptr>
						HttpServerResponseImpl &write( BytePtr first, BytePtr last ) {
							m_body.insert( std::end( m_body ), first, last );
							return *this;
						}

						template<size_t N>
						HttpServerResponseImpl &write( char const ( &buff )[N] ) {
							static_assert( N > 0, "Not sure what to do with an empty buff" );
							return write( buff, buff + ( N - 1 ) );
						}

						template<typename Container,
						         std::enable_if_t<daw::traits::is_container_like_v<Container>, std::nullptr_t> = nullptr>
						HttpServerResponseImpl &write( Container &&container ) {
							static_assert( sizeof( *std::cbegin( container ) ), "Data in container must be byte sized" );
							return this->write( std::begin( container ), std::end( container ) );
						}

						HttpServerResponseImpl &end( );

						template<typename... Args, std::enable_if_t<( sizeof...( Args ) > 0 ), std::nullptr_t> = nullptr>
						HttpServerResponseImpl &end( Args &&... args ) {
							this->write( std::forward<Args>( args )... );
							return this->end( );
						}

						void close( bool send_response = true );
						void start( );

						HttpHeaders &headers( );
						HttpHeaders const &headers( ) const;

						daw::nodepp::base::data_t const &body( ) const;

						HttpServerResponseImpl &send_status( uint16_t status_code = 200 );
						HttpServerResponseImpl &send_status( uint16_t status_code, daw::string_view status_msg );
						HttpServerResponseImpl &send_headers( );
						HttpServerResponseImpl &send_body( );
						HttpServerResponseImpl &clear_body( );
						bool send( );
						HttpServerResponseImpl &reset( );
						bool is_open( );
						bool is_closed( ) const;
						bool can_write( ) const;
						HttpServerResponseImpl &add_header( daw::string_view header_name, daw::string_view header_value );
						HttpServerResponseImpl &prepare_raw_write( size_t content_length );
						HttpServerResponseImpl &write_file( string_view file_name );

						HttpServerResponseImpl &write_file_async( string_view file_name );
					}; // struct HttpServerResponseImpl
				}    // namespace impl
			}      // namespace http
		}        // namespace lib
	}          // namespace nodepp
} // namespace daw
