// The MIT License (MIT)
//
// Copyright (c) 2014-2016 Darrell Wright
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

#include <boost/utility/string_view.hpp>
#include <string>
#include <vector>

#include <daw/json/daw_json_link.h>

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct HttpHeader : public daw::json::JsonLink<HttpHeader> {
					std::string name;
					std::string value;

					HttpHeader( );

					HttpHeader( boost::string_view Name, boost::string_view Value );

					~HttpHeader( );

					HttpHeader( HttpHeader const & ) = default;

					HttpHeader( HttpHeader && ) = default;

					HttpHeader &operator=( HttpHeader const & ) = default;

					HttpHeader &operator=( HttpHeader && ) = default;

					std::string to_string( ) const;

					bool empty( ) const noexcept;
				};

				struct HttpHeaders: public daw::json::JsonLink<HttpHeaders> {
					std::vector<HttpHeader> headers;

					HttpHeaders( );

					HttpHeaders( std::initializer_list<HttpHeader> values );

					~HttpHeaders( );

					HttpHeaders( HttpHeaders const & ) = default;

					HttpHeaders( HttpHeaders && ) = default;

					HttpHeaders &operator=( HttpHeaders const & ) = default;

					HttpHeaders &operator=( HttpHeaders && ) = default;

					std::vector<HttpHeader>::iterator begin( ) noexcept;

					std::vector<HttpHeader>::iterator end( ) noexcept;

					std::vector<HttpHeader>::const_iterator cbegin( ) const noexcept;

					std::vector<HttpHeader>::const_iterator cend( ) const noexcept;

					std::vector<HttpHeader>::iterator find( boost::string_view header_name );

					std::vector<HttpHeader>::const_iterator find( boost::string_view header_name ) const;

					std::string const &operator[]( boost::string_view header_name ) const;

					std::string &operator[]( boost::string_view header_name );

					std::string const &at( boost::string_view header_name ) const;

					std::string &at( boost::string_view header_name );

					bool exits( boost::string_view header_name ) const;

					std::string to_string( );

					HttpHeaders &add( std::string header_name, std::string header_value );
				};
			}    // namespace http
		}    // namespace lib
	}    // namespace nodepp
}    // namespace daw

