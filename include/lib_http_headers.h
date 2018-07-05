// The MIT License (MIT)
//
// Copyright (c) 2014-2017 Darrell Wright
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

#include <string>
#include <vector>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_key_value.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				using HttpHeader = daw::nodepp::base::key_value_t;

				struct HttpHeaders : public daw::json::daw_json_link<HttpHeaders> {
					using value_type = std::string;
					using iterator = typename std::vector<HttpHeader>::iterator;
					using const_iterator =
					  typename std::vector<HttpHeader>::const_iterator;
					using reference = value_type &;
					using const_reference = value_type const &;

					std::vector<HttpHeader> headers;

					HttpHeaders( std::initializer_list<HttpHeader> values );
					~HttpHeaders( ) = default;

					HttpHeaders( ) = default;
					HttpHeaders( HttpHeaders const & ) = default;
					HttpHeaders( HttpHeaders && ) noexcept = default;
					HttpHeaders &operator=( HttpHeaders const & ) = default;
					HttpHeaders &operator=( HttpHeaders && ) noexcept = default;

					iterator begin( ) noexcept;
					iterator end( ) noexcept;
					const_iterator cbegin( ) const noexcept;
					const_iterator cend( ) const noexcept;
					iterator find( daw::string_view header_name );
					const_iterator find( daw::string_view header_name ) const;
					const_reference operator[]( daw::string_view header_name ) const;
					reference operator[]( daw::string_view header_name );
					const_reference at( daw::string_view header_name ) const;
					reference at( daw::string_view header_name );
					bool exits( daw::string_view header_name ) const;
					size_t size( ) const noexcept;
					std::string to_string( );

					HttpHeaders &add( std::string header_name, std::string header_value );

					static void json_link_map( );
				};
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
