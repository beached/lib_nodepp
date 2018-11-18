// The MIT License (MIT)
//
// Copyright (c) 2017-2018 Darrell Wright
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

#include <daw/daw_string_view.h>
#include <memory>

#include "lib_http_parser.h"
#include "lib_http_parser_impl.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				std::shared_ptr<HttpAbsoluteUrlPath>
				parse_url_path( daw::string_view path ) {
					try {
						return std::make_shared<HttpAbsoluteUrlPath>(
						  parse::http_absolute_url_path_parser( path ) );
					} catch( daw::parser::ParserException const & ) { return nullptr; }
				}

				HttpClientRequest parse_http_request( daw::string_view str ) {
					return HttpClientRequest( parse::http_request_parser( str ) );
				}

				std::shared_ptr<impl::HttpUrlImpl>
				parse_url( daw::string_view url_string ) {
					try {
						return std::make_shared<impl::HttpUrlImpl>(
						  parse::http_url_parser( url_string ) );
					} catch( daw::parser::ParserException const & ) { return nullptr; }
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
