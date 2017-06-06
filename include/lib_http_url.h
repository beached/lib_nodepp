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

#include "base_types.h"
#include "lib_http_parser.h"
#include <boost/optional.hpp>
#include <boost/utility/string_view.hpp>
#include <daw/json/daw_json_link.h>
#include <memory>
#include <string>

namespace daw {
	namespace nodepp {
		namespace base {
			namespace json {
				struct JsonLink;
			} // namespace json
		}     // namespace base
		namespace lib {
			namespace http {
				struct UrlAuthInfo : public daw::json::JsonLink<UrlAuthInfo> {
					std::string username;
					std::string password;

					UrlAuthInfo( );
					UrlAuthInfo( std::string UserName, std::string Password );
					~UrlAuthInfo( );

					UrlAuthInfo( UrlAuthInfo const &other );
					UrlAuthInfo( UrlAuthInfo &&other );
					UrlAuthInfo &operator=( UrlAuthInfo const &rhs );
					UrlAuthInfo &operator=( UrlAuthInfo &&rhs );

				  private:
					void set_links( );
				}; // struct UrlAuthInfo

				std::string to_string( UrlAuthInfo const &auth );
				std::ostream &operator<<( std::ostream &os, UrlAuthInfo const &auth );

				struct HttpUrlQueryPair : public daw::json::JsonLink<HttpUrlQueryPair> {
					std::string name;
					boost::optional<std::string> value;

					HttpUrlQueryPair( );
					HttpUrlQueryPair( std::pair<std::string, boost::optional<std::string>> const &vals );
					~HttpUrlQueryPair( );

					HttpUrlQueryPair( HttpUrlQueryPair const &other );
					HttpUrlQueryPair( HttpUrlQueryPair &&other );
					HttpUrlQueryPair &operator=( HttpUrlQueryPair const &rhs );
					HttpUrlQueryPair &operator=( HttpUrlQueryPair &&rhs );

				  private:
					void set_links( );
				}; // HttpUrlQueryPair

				struct HttpAbsoluteUrlPath : public daw::json::JsonLink<HttpAbsoluteUrlPath> {
					std::string path;
					boost::optional<std::vector<HttpUrlQueryPair>> query;
					boost::optional<std::string> fragment;

					HttpAbsoluteUrlPath( );
					~HttpAbsoluteUrlPath( );

					HttpAbsoluteUrlPath( HttpAbsoluteUrlPath const &other );
					HttpAbsoluteUrlPath( HttpAbsoluteUrlPath &&other );
					HttpAbsoluteUrlPath &operator=( HttpAbsoluteUrlPath const &rhs );
					HttpAbsoluteUrlPath &operator=( HttpAbsoluteUrlPath &&rhs );

				  private:
					void set_links( );
				}; // HttpAbsoluteUrl

				std::string to_string( HttpAbsoluteUrlPath const &url_path );
				std::ostream &operator<<( std::ostream &os, HttpAbsoluteUrlPath const &url_path );

				namespace impl {
					struct HttpUrlImpl : public daw::json::JsonLink<HttpUrlImpl> {
						std::string scheme;
						boost::optional<UrlAuthInfo> auth_info;
						std::string host;
						boost::optional<uint16_t> port;
						boost::optional<HttpAbsoluteUrlPath> path;

						HttpUrlImpl( );
						~HttpUrlImpl( );

						HttpUrlImpl( HttpUrlImpl const &other );
						HttpUrlImpl( HttpUrlImpl &&other );
						HttpUrlImpl &operator=( HttpUrlImpl const &rhs );
						HttpUrlImpl &operator=( HttpUrlImpl &&rhs );

					  private:
						void set_links( );
					}; // HttpUrlImpl
				}      // namespace impl

				std::string to_string( daw::nodepp::lib::http::impl::HttpUrlImpl const &url );
				std::string to_string( daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &operator<<( std::ostream &os, daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &operator<<( std::ostream &os, daw::nodepp::lib::http::impl::HttpUrlImpl const &url );

			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
