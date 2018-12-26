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

#include <memory>
#include <optional>
#include <string>

#include <daw/daw_static_array.h>
#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_types.h"
#include "lib_http_parser.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct UrlAuthInfo : public daw::json::daw_json_link<UrlAuthInfo> {
					std::string username{};
					std::string password{};

					UrlAuthInfo( std::string UserName, std::string Password );

					UrlAuthInfo( ) = default;

					static void json_link_map( );
				}; // struct UrlAuthInfo

				std::string to_string( UrlAuthInfo const &auth );
				std::ostream &operator<<( std::ostream &os, UrlAuthInfo const &auth );

				struct HttpUrlQueryPair
				  : public daw::json::daw_json_link<HttpUrlQueryPair> {
					std::string name{};
					std::optional<std::string> value{};

					explicit HttpUrlQueryPair(
					  std::pair<std::string, std::optional<std::string>> const &vals );
					HttpUrlQueryPair( ) = default;

					static void json_link_map( );
				}; // HttpUrlQueryPair

				struct HttpAbsoluteUrlPath
				  : public daw::json::daw_json_link<HttpAbsoluteUrlPath> {
					std::string path;
					//					std::optional<std::vector<HttpUrlQueryPair>> query;
					std::vector<HttpUrlQueryPair> query;
					std::optional<std::string> fragment;

					static void json_link_map( );
					bool query_exists( daw::string_view name ) const noexcept;
					std::optional<std::string> query_get( daw::string_view name ) const;
				}; // HttpAbsoluteUrl

				std::string to_string( HttpAbsoluteUrlPath const &url_path );
				std::ostream &operator<<( std::ostream &os,
				                          HttpAbsoluteUrlPath const &url_path );

				namespace impl {
					struct HttpUrlImpl : public daw::json::daw_json_link<HttpUrlImpl> {
						std::string scheme;
						std::optional<UrlAuthInfo> auth_info;
						std::string host;
						std::optional<uint16_t> port;
						std::optional<HttpAbsoluteUrlPath> path;

						static void json_link_map( );
					}; // HttpUrlImpl
				}    // namespace nss_impl

				using HttpUrl =
				  std::shared_ptr<daw::nodepp::lib::http::impl::HttpUrlImpl>;

				std::string
				to_string( daw::nodepp::lib::http::impl::HttpUrlImpl const &url );
				std::string to_string( daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &operator<<( std::ostream &os,
				                          daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &
				operator<<( std::ostream &os,
				            daw::nodepp::lib::http::impl::HttpUrlImpl const &url );

			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
