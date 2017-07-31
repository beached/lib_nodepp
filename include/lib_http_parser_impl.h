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

#include <iterator>
#include <utility>
#include <vector>

#include <daw/daw_parser_addons.h>
#include <daw/daw_parser_helper.h>
#include <daw/daw_parser_helper_sv.h>

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
						daw::parser::find_result_t<char const *> path_parser( daw::string_view str,
						                                                      std::string &result );
						daw::nodepp::lib::http::HttpUrlQueryPair parse_query_pair( daw::string_view str );
						std::vector<daw::nodepp::lib::http::HttpUrlQueryPair> parse_query_pairs( daw::string_view str );

						daw::parser::find_result_t<char const *> query_parser( daw::string_view str,
						                                                       std::vector<HttpUrlQueryPair> &result );
						daw::parser::find_result_t<char const *>
						fragment_parser( daw::string_view str, boost::optional<std::string> &result );

						daw::parser::find_result_t<char const *>
						absolute_url_path_parser( daw::string_view str, boost::optional<HttpAbsoluteUrlPath> &result );

						daw::parser::find_result_t<char const *> request_line_parser( daw::string_view str, HttpRequestLine &result );
						std::pair<std::string, std::string> header_pair_parser( daw::string_view str );
						daw::parser::find_result_t<char const *> url_scheme_parser( daw::string_view str, daw::string_view result );
						daw::parser::find_result_t<char const *> url_auth_parser( daw::string_view str, boost::optional<UrlAuthInfo> &result );

						daw::parser::find_result_t<char const *>
						http_method_parser( daw::string_view str,
						                    daw::nodepp::lib::http::HttpClientRequestMethod &result );

						daw::parser::find_result_t<char const *> http_version_parser( daw::string_view str,
						                                                              daw::string_view result );

						daw::parser::find_result_t<char const *>
						header_parser( daw::string_view str, http::impl::HttpClientRequestImpl::headers_t &result );

						daw::parser::find_result_t<char const *>
						request_parser( daw::string_view str, http::impl::HttpClientRequestImpl &result );

						daw::parser::find_result_t<char const *> url_parser( daw::string_view str,
						                                                     http::impl::HttpUrlImpl &result );

						template<typename ForwardIterator>
						auto url_host_parser( ForwardIterator first, ForwardIterator last, std::string &result ) {
							static auto const not_valid = parser::in( "()<>@,;:\\\"/[]?={} \x09" );
							static auto const is_valid = parser::negate( not_valid );

							auto host_bounds = parser::from_to( first, last, is_valid, not_valid );
							result = host_bounds.as_string( );

							return host_bounds;
						}

						template<typename ForwardIterator>
						auto url_port_parser( ForwardIterator first, ForwardIterator last,
						                      boost::optional<uint16_t> &result ) {
							if( !parser::is_a( *first, ':' ) ) {
								result = boost::optional<uint16_t>{};
								return parser::make_find_result( first, last, false );
							}
							using value_t = daw::traits::root_type_t<decltype( *first )>;
							auto port_bounds = parser::from_to( std::next( first ), last, &parser::is_number<value_t>,
							                                    parser::negate( &parser::is_number<value_t> ) );
							parser::assert_not_empty( port_bounds.first, port_bounds.last );
							parser::parse_unsigned_int( port_bounds.first, port_bounds.last, *result );
							return port_bounds;
						}

					} // namespace impl

					template<typename ForwardIterator>
					auto http_absolute_url_path_parser( ForwardIterator first, ForwardIterator last ) {
						boost::optional<daw::nodepp::lib::http::HttpAbsoluteUrlPath> result;
						impl::absolute_url_path_parser( daw::make_string_view_it( first, last ), result );
						if( !result ) {
							throw parser::ParserException{};
						}
						return *result;
					}

					template<typename ForwardIterator>
					auto http_request_parser( ForwardIterator first, ForwardIterator last ) {
						daw::nodepp::lib::http::impl::HttpClientRequestImpl result;
						impl::request_parser( daw::make_string_view_it( first, last ), result );
						return result;
					}

					template<typename ForwardIterator>
					auto http_url_parser( ForwardIterator first, ForwardIterator last ) {
						daw::nodepp::lib::http::impl::HttpUrlImpl result;
						impl::url_parser( daw::make_string_view_it( first, last ), result );
						return result;
					}
				} // namespace parse
			}     // namespace http
		}         // namespace lib
	}             // namespace nodepp
} // namespace daw
