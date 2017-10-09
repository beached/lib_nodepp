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

#include <string>
#include <vector>

#include <daw/daw_string.h>

#include "lib_http_headers.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {

				HttpHeaders::iterator HttpHeaders::begin( ) noexcept {
					return headers.begin( );
				}

				HttpHeaders::iterator HttpHeaders::end( ) noexcept {
					return headers.end( );
				}

				HttpHeaders::const_iterator HttpHeaders::cbegin( ) const noexcept {
					return headers.cbegin( );
				}

				HttpHeaders::const_iterator HttpHeaders::cend( ) const noexcept {
					return headers.cend( );
				}

				void HttpHeaders::json_link_map( ) {
					link_json_object_array( "headers", headers );
				}

				HttpHeaders::HttpHeaders( std::initializer_list<HttpHeader> values )
				  : headers{std::begin( values ), std::end( values )} {}

				HttpHeaders::iterator HttpHeaders::find( daw::string_view header_name ) {
					auto it = std::find_if( std::begin( headers ), std::end( headers ), [&header_name]( HttpHeader const &item ) {
						return 0 == header_name.compare( item.key );
					} );
					return it;
				}

				HttpHeaders::const_iterator HttpHeaders::find( daw::string_view header_name ) const {
					auto it = std::find_if( std::begin( headers ), std::end( headers ), [&header_name]( HttpHeader const &item ) {
						return 0 == header_name.compare( item.key );
					} );
					return it;
				}

				HttpHeaders::const_reference HttpHeaders::operator[]( daw::string_view header_name ) const {
					return find( header_name )->value;
				}

				HttpHeaders::reference HttpHeaders::operator[]( daw::string_view header_name ) {
					auto it = find( header_name );
					if( it == headers.end( ) ) {
						it = headers.emplace( headers.end( ), header_name, "" );
					}
					return it->value;
				}

				bool HttpHeaders::exits( daw::string_view header_name ) const {
					return find( header_name ) != headers.cend( );
				}

				HttpHeaders::const_reference HttpHeaders::at( daw::string_view header_name ) const {
					auto it = HttpHeaders::find( header_name );
					daw::exception::daw_throw_on_false<std::out_of_range>( it != std::end( headers ),
					                                                       header_name.to_string( ) + " is not a valid header" );

					return it->value;
				}

				HttpHeaders::reference HttpHeaders::at( daw::string_view header_name ) {
					auto it = HttpHeaders::find( header_name );
					daw::exception::daw_throw_on_false<std::out_of_range>( it != std::end( headers ),
					                                                       header_name.to_string( ) + " is not a valid header" );
					return it->value;
				}

				std::string HttpHeaders::to_string( ) {
					std::stringstream ss;
					for( auto const &header : headers ) {
						ss << header.to_string( ) << "\r\n";
					}
					return ss.str( );
				}

				HttpHeaders &HttpHeaders::add( daw::string_view header_name, daw::string_view header_value ) {
					headers.emplace_back( header_name, header_value );
					return *this;
				}

				size_t HttpHeaders::size( ) const noexcept {
					return headers.size( );
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
