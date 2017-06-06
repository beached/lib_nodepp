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

#include <memory>

#include <daw/daw_parser_helper.h>

#include "lib_http_parser.h"
#include "lib_http_parser_impl.h"
#include "lib_http_request.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				HttpClientRequest parse_http_request( daw::nodepp::base::data_t::iterator first,
				                                      daw::nodepp::base::data_t::iterator last ) {
					try {
						return std::make_shared<impl::HttpClientRequestImpl>(
						    parse::http_request_parser( first, last ) );
					} catch( daw::parser::ParserException const & ) { return nullptr; }
				}

				std::shared_ptr<daw::nodepp::lib::http::HttpAbsoluteUrlPath> parse_url_path( boost::string_view path ) {
					try {
						return std::make_shared<daw::nodepp::lib::http::HttpAbsoluteUrlPath>(
						    daw::nodepp::lib::http::parse::http_absolute_url_path_parser( path.begin( ),
						                                                                  path.end( ) ) );
					} catch( daw::parser::ParserException const & ) { return nullptr; }
				}
				std::shared_ptr<daw::nodepp::lib::http::impl::HttpUrlImpl> parse_url( boost::string_view url_string ) {
					try {
						return std::make_shared<daw::nodepp::lib::http::impl::HttpUrlImpl>(
						    daw::nodepp::lib::http::parse::http_url_parser( url_string.begin( ), url_string.end( ) ) );
					} catch( daw::parser::ParserException const & ) { return nullptr; }
				}
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
