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

#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <daw/daw_common_mixins.h>
#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_key_value.h"
#include "base_types.h"
#include "lib_http_parser.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct HttpAbsoluteUrlPath;

				enum class HttpClientRequestMethod : uint_fast8_t {
					Options = 1,
					Get,
					Head,
					Post,
					Put,
					Delete,
					Trace,
					Connect,
					Any
				};

				inline std::string to_string( HttpClientRequestMethod method ) {
					switch( method ) {
					case HttpClientRequestMethod::Get:
						return "GET";
					case HttpClientRequestMethod::Post:
						return "POST";
					case HttpClientRequestMethod::Connect:
						return "CONNECT";
					case HttpClientRequestMethod::Delete:
						return "DELETE";
					case HttpClientRequestMethod::Head:
						return "HEAD";
					case HttpClientRequestMethod::Options:
						return "OPTIONS";
					case HttpClientRequestMethod::Put:
						return "PUT";
					case HttpClientRequestMethod::Trace:
						return "TRACE";
					case HttpClientRequestMethod::Any:
						return "ANY";
					default:
						daw::exception::daw_throw_unexpected_enum( );
					}
				}

				std::ostream &operator<<( std::ostream &os,
				                          HttpClientRequestMethod const &method );

				std::istream &operator>>( std::istream &is,
				                          HttpClientRequestMethod &method );

				std::string to_string( HttpClientRequestMethod method );

				namespace impl {
					constexpr bool is_equal_nc( daw::string_view lhs,
					                            daw::string_view rhs ) noexcept {
						if( lhs.size( ) != rhs.size( ) ) {
							return false;
						}
						bool result = true;
						for( size_t n = 0; n < lhs.size( ); ++n ) {
							result &= ( static_cast<uint8_t>( lhs[n] ) |
							            static_cast<uint8_t>( ' ' ) ) ==
							          ( static_cast<uint8_t>( rhs[n] ) |
							            static_cast<uint8_t>( ' ' ) );
						}
						return result;
					}
				} // namespace impl

				constexpr HttpClientRequestMethod
				from_string( daw::tag_t<HttpClientRequestMethod>,
				             daw::string_view method ) {

					if( impl::is_equal_nc( "get", method ) ) {
						return HttpClientRequestMethod::Get;
					}

					if( impl::is_equal_nc( "post", method ) ) {
						return HttpClientRequestMethod::Post;
					}

					if( impl::is_equal_nc( "connect", method ) ) {
						return HttpClientRequestMethod::Connect;
					}

					if( impl::is_equal_nc( "delete", method ) ) {
						return HttpClientRequestMethod::Delete;
					}

					if( impl::is_equal_nc( "head", method ) ) {
						return HttpClientRequestMethod::Head;
					}

					if( impl::is_equal_nc( "options", method ) ) {
						return HttpClientRequestMethod::Options;
					}

					if( impl::is_equal_nc( "put", method ) ) {
						return HttpClientRequestMethod::Put;
					}

					if( impl::is_equal_nc( "trace", method ) ) {
						return HttpClientRequestMethod::Trace;
					}

					if( impl::is_equal_nc( "any", method ) ) {
						return HttpClientRequestMethod::Any;
					}
					daw::exception::daw_throw_unexpected_enum( );
				}

				struct HttpRequestLine {
					std::string version;
					HttpAbsoluteUrlPath url;
					HttpClientRequestMethod method;
				}; // HttpRequestLine

				namespace impl {
					struct req_method_maker_t {
						constexpr HttpClientRequestMethod operator( )( char const *ptr,
						                                               size_t sz ) const {
							return from_string( daw::tag<HttpClientRequestMethod>,
							                    daw::string_view( ptr, sz ) );
						}
					};
				} // namespace impl

				inline auto describe_json_class( HttpRequestLine ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "version";
					static constexpr char const n1[] = "url";
					static constexpr char const n2[] = "method";
					return class_description_t<json_string<n0>,
					                           json_class<n1, HttpAbsoluteUrlPath>,
					                           json_enum<n2, HttpClientRequestMethod>>{};
				}

				inline auto to_json_data( HttpRequestLine const &value ) noexcept {
					return std::forward_as_tuple( value.version, value.url,
					                              value.method );
				}

				struct HttpClientRequestBody {
					std::string content_type;
					std::string content;
				}; // struct HttpClientRequestBody

				inline auto describe_json_class( HttpClientRequestBody ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "content_type";
					static constexpr char const n1[] = "content";
					return class_description_t<json_string<n0>, json_string<n1>>{};
				}

				inline auto
				to_json_data( HttpClientRequestBody const &value ) noexcept {
					return std::forward_as_tuple( value.content_type, value.content );
				}

				struct HttpClientRequestHeader {
					std::string first{};
					std::string second{};

					HttpClientRequestHeader( ) = default;
					HttpClientRequestHeader( std::string First, std::string Second );
					HttpClientRequestHeader( daw::string_view First,
					                         daw::string_view Second );
					explicit HttpClientRequestHeader(
					  std::pair<std::string, std::string> values );

					HttpClientRequestHeader &
					operator=( std::pair<std::string, std::string> values );
				};

				inline auto describe_json_class( HttpClientRequestHeader ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "first";
					static constexpr char const n1[] = "second";
					return class_description_t<json_string<n0>, json_string<n1>>{};
				}

				inline auto
				to_json_data( HttpClientRequestHeader const &value ) noexcept {
					return std::forward_as_tuple( value.first, value.second );
				}

				bool operator==( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept;
				bool operator!=( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept;
				bool operator>( HttpClientRequestHeader const &lhs,
				                HttpClientRequestHeader const &rhs ) noexcept;
				bool operator<( HttpClientRequestHeader const &lhs,
				                HttpClientRequestHeader const &rhs ) noexcept;
				bool operator>=( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept;
				bool operator<=( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept;

				struct HttpClientRequestHeaders {
					using value_type = HttpClientRequestHeader;
					using reference = std::string &;
					using const_reference = std::string const &;
					using values_type = std::vector<value_type>;
					using iterator = typename values_type::iterator;
					using const_iterator = typename values_type::const_iterator;
					using key_type = std::string;
					using mapped_type = std::string;
					using size_type = typename values_type::size_type;

				private:
					values_type headers;

				public:
					friend inline auto
					to_json_data( HttpClientRequestHeaders const &value ) noexcept {
						return std::forward_as_tuple( value.headers );
					}

					HttpClientRequestHeaders( ) = default;
					explicit HttpClientRequestHeaders( values_type h );

					inline auto begin( ) {
						return headers.begin( );
					}

					inline auto begin( ) const {
						return headers.begin( );
					}

					inline auto cbegin( ) const {
						return headers.cbegin( );
					}

					inline auto end( ) {
						return headers.end( );
					}

					inline auto end( ) const {
						return headers.end( );
					}

					inline auto cend( ) const {
						return headers.cend( );
					}

					iterator find( daw::string_view key );
					const_iterator find( daw::string_view key ) const;

					reference operator[]( key_type key );
					const_reference operator[]( key_type key ) const;

					template<typename Iterator>
					iterator insert( Iterator where, value_type &&value ) {
						return headers.insert( where, std::forward<value_type>( value ) );
					}

					template<typename Iterator, typename... Args>
					iterator insert( Iterator where, Args &&... args ) {
						return headers.emplace( where, std::forward<Args>( args )... );
					}

					template<typename Name, typename Value>
					iterator add( Name &&name, Value &&value ) {
						return headers.emplace( headers.cend( ), std::forward<Name>( name ),
						                        std::forward<Value>( value ) );
					}

					iterator add( HttpClientRequestHeader &&h ) {
						return headers.insert( headers.cend( ),
						                       std::forward<HttpClientRequestHeader>( h ) );
					}

					size_type size( ) const noexcept;
					bool empty( ) const noexcept;
				}; // HttpClientRequestHeaders

				inline auto describe_json_class( HttpClientRequestHeaders ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "headers";
					return class_description_t<
					  json_array<n0, std::vector<HttpClientRequestHeader>,
					             json_class<no_name, HttpClientRequestHeader>>>{};
				}

				struct HttpClientRequest {
					using headers_t = HttpClientRequestHeaders;

					daw::nodepp::lib::http::HttpRequestLine request_line{};
					headers_t headers{};
					std::optional<daw::nodepp::lib::http::HttpClientRequestBody> body{};

					std::vector<base::key_value_t>
					get_parameters( daw::string_view prefix ) const;
				}; // struct HttpClientRequest

				inline auto describe_json_class( HttpClientRequest ) noexcept {
					using namespace daw::json;
					static constexpr char const n0[] = "request_line";
					static constexpr char const n1[] = "headers";
					static constexpr char const n2[] = "body";
					return class_description_t<
					  json_class<n0, HttpRequestLine>,
					  json_class<n1, HttpClientRequestHeaders>,
					  json_nullable<json_class<n2, HttpClientRequestBody>>>{};
				}

				inline auto to_json_data( HttpClientRequest const &value ) noexcept {
					return std::forward_as_tuple( value.request_line, value.headers,
					                              value.body );
				}

				HttpClientRequest
				create_http_client_request( daw::string_view path,
				                            HttpClientRequestMethod const &method );
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
