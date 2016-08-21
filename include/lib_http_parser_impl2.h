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

#include <iterator>
#include <vector>

#include "daw_parser_helper.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace parse {
					namespace impl {
						template<typename ForwardIterator, typename Result>
							auto path_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								parser::assert_not_empty( first, last );
								// char_( '/' )>> *(~char_( " ?#" ));
								static auto const is_first = []( auto const & v ) {
									return parser::is_a( v, '/' );
								};

								static auto const is_last = []( auto const & v ) {
									return parser::is_a( v, ' ', '?', '#' );
								};

								auto bounds = parser::from_to( first, last, is_first, is_last ).as<Result>( );
								return bounds;
							}

						template<typename ForwardIterator>
							auto parse_query_pair( ForwardIterator first, ForwardIterator last ) {
								parser::assert_not_empty( first, last );
								auto has_value = parser::until_value( first, last, '=' );

								HttpUrlQueryPair result;
								result.name = has_value.as<std::string>( );
								if( has_value ) {
									result.value = std::string( std::next( has_value.last ), last );
								}
								return result;
							}

						template<typename ForwardIterator>
							auto parse_query_pairs( ForwardIterator first, ForwardIterator last ) {
								parser::assert_not_empty( first, last );
								std::vector<HttpUrlQueryPair> pairs;
								{
									auto result = parser::until_value( first, last, '&' );
									while( result ) {
										pairs.push_back( parse_query_pair( result.first, result.last ) );
										result = parser::until_value( std::next( result.last ), last, '&' );
									}
									if( result.first != result.last ) {
										pairs.push_back( parse_query_pair( result.first, result.last ) );
									}
								}
								return pairs;
							}

						template<typename ForwardIterator, typename Result>
							auto query_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								// lit( '?' )>> query_pair>> *((qi::lit( ';' ) | '&')>> query_pair);
								parser::assert_not_empty( first, last );
								static auto const is_first = []( auto const & v ) {
									return parser::is_a( v, '?' );
								};

								static auto const is_last = []( auto const & v ) {
									return parser::is_a( v, '#', ' ' );
								};

								auto bounds = parser::from_to( first, last, is_first, is_last );
								result = parse_query_pairs( bounds.first, bounds.last );
								return bounds;
							}

						template<typename ForwardIterator, typename Result>
							auto fragment_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								// lit( '#' )>> *(~char_( " " ));
								auto bounds = parser::from_to( first, last, '#', ' ' );
								if( !bounds.empty( ) ) {
									result = bounds.as<std::string>( );
								}
								return bounds;
							}

						template<typename ForwardIterator, typename Result>
							auto absolute_url_path_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								parser::assert_not_empty( first, last );
								static auto const is_end_of_url = []( auto const & v ) { return parser::is_a( v, ' ' ); };

								auto url_bounds = parser::from_to( first, last, &parser::pred_true, is_end_of_url, true );

								auto bounds = impl::path_parser( url_bounds.first, url_bounds.last, result.path );
								bounds = impl::query_parser( bounds.last, url_bounds.last, result.query );
								impl::fragment_parser( bounds.last, url_bounds.last, result.fragment );
								return url_bounds;
							}

						template<typename ForwardIterator, typename Result>
							auto http_method_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								parser::assert_not_empty( first, last );
								auto const method_bounds = parser::from_to( first, last, &parser::pred_true, &parser::is_space, true );
								result = http_request_method_from_string( method_bounds.as<std::string>( ) );
								return method_bounds;
							};

						template<typename ForwardIterator, typename Result>
							auto http_version_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
								parser::assert_not_empty( first, last );
								auto version_bounds = parser::from_to( first, last, parser::negate( &parser::is_space ), &parser::pred_false );
								parser::assert_not_empty( version_bounds.first, version_bounds.last );

								auto bounds = parser::from_to( version_bounds.first, version_bounds.last, &parser::is_number, &parser::pred_false );
								result = bounds.as<std::string>( );
								return version_bounds;
							}

						template<typename ForwardIterator, typename Result>
							auto request_line_parser( ForwardIterator first, ForwardIterator last, Result & result ) {
								parser::assert_not_empty( first, last );
								using value_t = std::decay_t<std::remove_reference_t<decltype( *first )>>;
								value_t last_val = *first;
								auto req_bounds = parer::until( first, last, parser::is_crlf{ } );
								auto bounds = http_method_parser( req_bounds.first, req_bounds.last, result.method );
								bounds = absolute_url_path_parser( bounds.last, req_bounds.last, result.url );
								http_version_parser( bounds.last, req_bounds.last, result.version );
								return req_bounds;
							};


						template<typename ForwardIterator, typename Result>
						    auto http_token_parer( ForwardIterator first, ForwardIterator last, Result & result ) {
								// +(~char_( "()<>@,;:\\\"/[]?={} \x09" ));
								static std::string const non_tokens = "()<>@,;:\\\"/[]?={} \x09";
								parser::expect( *first, parser::in( non_tokens ) );
								return parser::until_values( first, last, non_tokens );
							}
					}    // namespace impl

					template<typename ForwardIterator>
						auto http_absolute_url_path_parser( ForwardIterator first, ForwardIterator last ) {
							daw::nodepp::lib::http::HttpAbsoluteUrlPath result;
							impl::absolute_url_path_parser( first, last, result );
							return result;
						}

					template<typename ForwardIterator>
						auto http_request_parser( ForwardIterator first, ForwardIterator last ) {
							daw::nodepp::lib::http::impl::HttpClientRequestImpl result;
							impl::request_line_parser( first, last, result );
							return result;
						}

					template<typename ForwardIterator>
					    auto http_url_parser( ForwardIterator first, ForwardIterator last ) {
							daw::nodepp::lib::http::impl::HttpUrlImpl result;

							return result;
						}
				} // namespace http
			}	// namespace parse
		}    // namespace lib
	}    // namespace nodepp
}    // namespace daw

