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

#include <boost/optional.hpp>
#include <memory>
#include <string>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_types.h"
#include "lib_http_parser.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct UrlAuthInfo : public daw::json::daw_json_link<UrlAuthInfo> {
					std::string username;
					std::string password;

					UrlAuthInfo( daw::string_view UserName, daw::string_view Password );

					UrlAuthInfo( ) = default;
					~UrlAuthInfo( ) = default;
					UrlAuthInfo( UrlAuthInfo const & ) = default;
					UrlAuthInfo( UrlAuthInfo && ) = default;
					UrlAuthInfo &operator=( UrlAuthInfo const & ) = default;
					UrlAuthInfo &operator=( UrlAuthInfo && ) = default;

					static void json_link_map( );
				}; // struct UrlAuthInfo

				std::string to_string( UrlAuthInfo const &auth );
				std::ostream &operator<<( std::ostream &os, UrlAuthInfo const &auth );

				struct HttpUrlQueryPair : public daw::json::daw_json_link<HttpUrlQueryPair> {
					std::string name;
					boost::optional<std::string> value;

					explicit HttpUrlQueryPair( std::pair<std::string, boost::optional<std::string>> const &vals );

					HttpUrlQueryPair( ) = default;
					~HttpUrlQueryPair( ) = default;
					HttpUrlQueryPair( HttpUrlQueryPair const & ) = default;
					HttpUrlQueryPair( HttpUrlQueryPair && ) = default;
					HttpUrlQueryPair &operator=( HttpUrlQueryPair const & ) = default;
					HttpUrlQueryPair &operator=( HttpUrlQueryPair && ) = default;

					static void json_link_map( );
				}; // HttpUrlQueryPair

				struct HttpAbsoluteUrlPath : public daw::json::daw_json_link<HttpAbsoluteUrlPath> {
					std::string path;
					//					boost::optional<std::vector<HttpUrlQueryPair>> query;
					std::vector<HttpUrlQueryPair> query;
					boost::optional<std::string> fragment;

					static void json_link_map( );
					bool query_exists( daw::string_view name ) const noexcept;
					boost::optional<std::string> query_get( daw::string_view name ) const;
				}; // HttpAbsoluteUrl

				std::string to_string( HttpAbsoluteUrlPath const &url_path );
				std::ostream &operator<<( std::ostream &os, HttpAbsoluteUrlPath const &url_path );

				namespace impl {
					struct HttpUrlImpl : public daw::json::daw_json_link<HttpUrlImpl> {
						std::string scheme;
						boost::optional<UrlAuthInfo> auth_info;
						std::string host;
						boost::optional<uint16_t> port;
						boost::optional<HttpAbsoluteUrlPath> path;

						static void json_link_map( );
					}; // HttpUrlImpl
				}      // namespace impl

				using HttpUrl = std::shared_ptr<daw::nodepp::lib::http::impl::HttpUrlImpl>;

				std::string to_string( daw::nodepp::lib::http::impl::HttpUrlImpl const &url );
				std::string to_string( daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &operator<<( std::ostream &os, daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &operator<<( std::ostream &os, daw::nodepp::lib::http::impl::HttpUrlImpl const &url );

			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
