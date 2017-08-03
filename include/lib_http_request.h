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
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <daw/daw_common_mixins.h>
#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_types.h"
#include "lib_http_parser.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				struct HttpAbsoluteUrlPath;

				enum class HttpClientRequestMethod { Options = 1, Get, Head, Post, Put, Delete, Trace, Connect, Any };

				std::ostream &operator<<( std::ostream &os, HttpClientRequestMethod const method );

				std::istream &operator>>( std::istream &is, HttpClientRequestMethod &method );

				std::string to_string( HttpClientRequestMethod method );

				namespace impl {
					constexpr bool is_equal_nc( daw::string_view lhs, daw::string_view rhs ) noexcept {
						if( lhs.size( ) != rhs.size( ) ) {
							return false;
						}
						bool result = true;
						for( size_t n = 0; n < lhs.size( ); ++n ) {
							result &= ( lhs[n] | ' ' ) == ( rhs[n] | ' ' );
						}
						return result;
					}
				} // namespace impl

				template<typename CharT, typename TraitsT>
				constexpr HttpClientRequestMethod http_request_method_from_string( daw::basic_string_view<CharT, TraitsT> method ) {
					if( impl::is_equal_nc( "get", method ) ) {
						return HttpClientRequestMethod::Get;
					} else if( impl::is_equal_nc( "post", method ) ) {
						return HttpClientRequestMethod::Post;
					} else if( impl::is_equal_nc( "connect", method ) ) {
						return HttpClientRequestMethod::Connect;
					} else if( impl::is_equal_nc( "delete", method ) ) {
						return HttpClientRequestMethod::Delete;
					} else if( impl::is_equal_nc( "head", method ) ) {
						return HttpClientRequestMethod::Head;
					} else if( impl::is_equal_nc( "options", method ) ) {
						return HttpClientRequestMethod::Options;
					} else if( impl::is_equal_nc( "put", method ) ) {
						return HttpClientRequestMethod::Put;
					} else if( impl::is_equal_nc( "trace", method ) ) {
						return HttpClientRequestMethod::Trace;
					} else if( impl::is_equal_nc( "any", method ) ) {
						return HttpClientRequestMethod::Any;
					}
					throw std::runtime_error( "unknown http request method" );
				}

				struct HttpRequestLine : public daw::json::daw_json_link<HttpRequestLine> {
					HttpClientRequestMethod method;
					HttpAbsoluteUrlPath url;
					std::string version;

					static void json_link_map( );
				}; // HttpRequestLine

				struct HttpClientRequestBody : public daw::json::daw_json_link<HttpClientRequestBody> {
					std::string content_type;
					std::string content;

					static void json_link_map( );
				}; // struct HttpClientRequestBody

				struct HttpClientRequestHeader: public daw::json::daw_json_link<HttpClientRequestHeader> {
					std::string first;
					std::string second;

					HttpClientRequestHeader( std::string First, std::string Second );
					HttpClientRequestHeader( std::pair<std::string, std::string> values );
					HttpClientRequestHeader &operator=( std::pair<std::string, std::string> values );

					HttpClientRequestHeader( ) = default;
					~HttpClientRequestHeader( ) = default;
					HttpClientRequestHeader( HttpClientRequestHeader const & ) = default;
					HttpClientRequestHeader( HttpClientRequestHeader && ) = default;
					HttpClientRequestHeader &operator=( HttpClientRequestHeader const & ) = default;
					HttpClientRequestHeader &operator=( HttpClientRequestHeader && ) = default;

					static void json_link_map( );
				};

				void swap( HttpClientRequestHeader &lhs, HttpClientRequestHeader &rhs ) noexcept;

				bool operator==( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept;
				bool operator!=( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept;
				bool operator>( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept;
				bool operator<( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept;
				bool operator>=( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept;
				bool operator<=( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept;

				struct HttpClientRequestHeaders : public daw::json::daw_json_link<HttpClientRequestHeaders> {
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
					explicit HttpClientRequestHeaders( values_type h );
					HttpClientRequestHeaders( ) = default;
					~HttpClientRequestHeaders( ) = default;
					HttpClientRequestHeaders( HttpClientRequestHeaders const & ) = default;
					HttpClientRequestHeaders( HttpClientRequestHeaders && ) = default;
					HttpClientRequestHeaders &operator=( HttpClientRequestHeaders const & ) = default;
					HttpClientRequestHeaders &operator=( HttpClientRequestHeaders && ) = default;

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
					iterator add( Name && name, Value && value ) {
						return headers.emplace( headers.cend( ), std::forward<Name>( name ),
						                        std::forward<Value>( value ) );
					}

					size_type size( ) const noexcept;
					bool empty( ) const noexcept;

					static void json_link_map( );
				}; // HttpClientRequestHeaders

				namespace impl {
					struct HttpClientRequestImpl : public daw::json::daw_json_link<HttpClientRequestImpl> {
						using headers_t = HttpClientRequestHeaders;

						daw::nodepp::lib::http::HttpRequestLine request_line;
						headers_t headers;
						boost::optional<daw::nodepp::lib::http::HttpClientRequestBody> body;

						static void json_link_map( );
					}; // struct HttpClientRequestImpl
				}      // namespace impl

				using HttpClientRequest = std::shared_ptr<daw::nodepp::lib::http::impl::HttpClientRequestImpl>;

				HttpClientRequest create_http_client_request( daw::string_view path,
				                                              HttpClientRequestMethod const &method );
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
