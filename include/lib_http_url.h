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

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_types.h"
#include "lib_http_parser.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct UrlAuthInfo {
					std::string username{};
					std::string password{};

					UrlAuthInfo( std::string UserName, std::string Password );

					UrlAuthInfo( ) = default;
				}; // struct UrlAuthInfo

				inline auto describe_json_class( UrlAuthInfo ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "username";
					static constexpr char const n1[] = "password";
					return class_description_t<json_string<n0>, json_string<n1>>{};
				}

				inline auto to_json_data( UrlAuthInfo const &value ) noexcept {
					return std::forward_as_tuple( value.username, value.password );
				}

				std::string to_string( UrlAuthInfo const &auth );
				std::ostream &operator<<( std::ostream &os, UrlAuthInfo const &auth );

				struct HttpUrlQueryPair {
					std::string name{};
					std::optional<std::string> value{};

					explicit HttpUrlQueryPair(
					  std::pair<std::string, std::optional<std::string>> const &vals );
					HttpUrlQueryPair( ) = default;

					static void json_link_map( );
				}; // HttpUrlQueryPair

				inline auto describe_json_class( HttpUrlQueryPair ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "name";
					static constexpr char const n1[] = "value";
					return class_description_t<json_string<n0>,
					                           json_string<n1, std::string, NullValueOpt::allowed>>{};
				}

				inline auto to_json_data( HttpUrlQueryPair const &value ) noexcept {
					return std::forward_as_tuple( value.name, value.value );
				}

				struct HttpAbsoluteUrlPath {
					std::string path;
					std::vector<HttpUrlQueryPair> query;
					std::optional<std::string> fragment;

					bool query_exists( daw::string_view name ) const noexcept;
					std::optional<std::string> query_get( daw::string_view name ) const;
				}; // HttpAbsoluteUrl

				inline auto describe_json_class( HttpAbsoluteUrlPath ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "path";
					static constexpr char const n1[] = "query";
					static constexpr char const n2[] = "fragment";
					return class_description_t<
					  json_string<n0>,
					  json_array<n1, std::vector<HttpUrlQueryPair>,
					             json_class<no_name, HttpUrlQueryPair>>,
					  json_string<n2, std::string, NullValueOpt::allowed>>{};
				}

				inline auto to_json_data( HttpAbsoluteUrlPath const &value ) noexcept {
					return std::forward_as_tuple( value.path, value.query,
					                              value.fragment );
				}

				std::string to_string( HttpAbsoluteUrlPath const &url_path );
				std::ostream &operator<<( std::ostream &os,
				                          HttpAbsoluteUrlPath const &url_path );

				namespace hp_impl {
					struct HttpUrlImpl {
						std::string scheme;
						std::optional<UrlAuthInfo> auth_info;
						std::string host;
						std::optional<uint16_t> port;
						std::optional<HttpAbsoluteUrlPath> path;
					}; // HttpUrlImpl

					inline auto describe_json_class( HttpUrlImpl ) noexcept {
						using namespace daw::json;
						static constexpr char const n0[] = "scheme";
						static constexpr char const n1[] = "auth_info";
						static constexpr char const n2[] = "host";
						static constexpr char const n3[] = "port";
						static constexpr char const n4[] = "path";
						return class_description_t<
						  json_string<n0>, json_class<n1, UrlAuthInfo>, json_string<n2>,
						  json_number<n3, uint16_t, NullValueOpt::allowed>,
						  json_class<n4, HttpAbsoluteUrlPath, NullValueOpt::allowed>>{};
					}

					inline auto to_json_data( HttpUrlImpl const &value ) noexcept {
						return std::forward_as_tuple( value.scheme, value.auth_info,
						                              value.host, value.port, value.path );
					}
				} // namespace hp_impl

				using HttpUrl =
				  std::shared_ptr<daw::nodepp::lib::http::hp_impl::HttpUrlImpl>;

				std::string
				to_string( daw::nodepp::lib::http::hp_impl::HttpUrlImpl const &url );
				std::string to_string( daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &operator<<( std::ostream &os,
				                          daw::nodepp::lib::http::HttpUrl const &url );
				std::ostream &
				operator<<( std::ostream &os,
				            daw::nodepp::lib::http::hp_impl::HttpUrlImpl const &url );

			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
