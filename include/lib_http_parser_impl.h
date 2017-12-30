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

#include "lib_http_request.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct HttpUrlQueryPair;
				struct HttpAbsoluteUrlPath;
				struct HttpRequestLine;
				struct UrlAuthInfo;

				namespace parse {
					namespace impl {
						HttpAbsoluteUrlPath absolute_url_path_parser( daw::string_view str );

						daw::string_view request_parser( daw::string_view str, http::HttpClientRequest &result );

						daw::string_view url_parser( daw::string_view str, http::impl::HttpUrlImpl &result );
					} // namespace impl

					daw::nodepp::lib::http::HttpAbsoluteUrlPath http_absolute_url_path_parser( daw::string_view str );
					daw::nodepp::lib::http::HttpClientRequest http_request_parser( daw::string_view str );
					daw::nodepp::lib::http::impl::HttpUrlImpl http_url_parser( daw::string_view str );
				} // namespace parse
			}   // namespace http
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
