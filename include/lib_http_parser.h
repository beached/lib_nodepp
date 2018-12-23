// The MIT License (MIT)
//
// Copyright (c) 2014-2018 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all
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

#include <memory>

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>
#include <string>

#include "base_types.h"
#include "base_url.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct HttpClientRequest;

				namespace impl {
					struct HttpUrlImpl;

					constexpr char make_nibble_from_hex( char c ) {
						switch( c ) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							return ( c - '0' );
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							return ( c - 'a' ) + 10;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							return ( c - 'A' ) + 10;
						default:
							daw::exception::daw_throw( "Invalid hex digit" );
						}
					}

					constexpr char make_hex( daw::string_view str ) {
						auto c =
						  static_cast<char>( static_cast<unsigned char>(
						                       make_nibble_from_hex( str.pop_front( ) ) )
						                     << 4u );
						c |= make_nibble_from_hex( str.front( ) );
						return c;
					}
				} // namespace nss_impl

				struct HttpAbsoluteUrlPath;

				template<typename CharT>
				std::basic_string<CharT>
				url_decode( daw::basic_string_view<CharT> str ) {
					auto result = std::basic_string<CharT>( );
					while( !str.empty( ) ) {
						result += str.pop_front( "%" );
						if( str.size( ) >= 2 ) {
							result += impl::make_hex( str.pop_front( 2 ) );
						}
					}
					return result;
				}

				HttpClientRequest parse_http_request( daw::string_view str );

				std::shared_ptr<HttpAbsoluteUrlPath>
				parse_url_path( daw::string_view path );

				std::shared_ptr<impl::HttpUrlImpl>
				parse_url( daw::string_view url_string );

				template<typename SizeT = uint16_t>
				struct basic_url_parser {
					using size_type = SizeT;
					constexpr size_type operator( )( base::uri_parts, daw::string_view,
					                                 size_type ) const;
				};

				// TODO
				// daw::nodepp::base::basic_uri_buffer<daw::nodepp::lib::http::basic_url_parser>
				// parse_url2( daw::string_view url_string );
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
