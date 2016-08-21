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
#include <utility>
#include <vector>

#include <daw/daw_parser_helper.h>

#include "lib_http_url.h"
#include "lib_http_request.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace parse {
					namespace impl {
						template<typename ForwardIterator>
							auto path_parser( ForwardIterator first, ForwardIterator last, std::string & result ) {
								parser::assert_not_empty( first, last );
								// char_( '/' )>> *(~char_( " ?#" ));
								static auto const is_first = []( auto const & v ) {
									return parser::is_a( v, '/' );
								};

								static auto const is_last = []( auto const & v ) {
									return parser::is_a( v, ' ', '?', '#' );
								};

								auto bounds = parser::from_to( first, last, is_first, is_last );
								result = bounds.as_string( );
								return bounds;
							}

						template<typename ForwardIterator>
							auto parse_query_pair( ForwardIterator first, ForwardIterator last ) {
								parser::assert_not_empty( first, last );
								auto has_value = parser::until_value( first, last, '=' );

								HttpUrlQueryPair result;
								result.name = has_value.as_string( );
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

						template<typename ForwardIterator>
							auto query_parser( ForwardIterator first, ForwardIterator last, boost::optional<std::vector<HttpUrlQueryPair>> & result ) {
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

						template<typename ForwardIterator>
							auto fragment_parser( ForwardIterator first, ForwardIterator last, boost::optional<std::string> & result ) {
								// lit( '#' )>> *(~char_( " " ));
								auto bounds = parser::from_to( first, last, '#', ' ' );
								if( !bounds.empty( ) ) {
									result = bounds.as_string( );
								}
								return bounds;
							}

						template<typename ForwardIterator>
							auto absolute_url_path_parser( ForwardIterator first, ForwardIterator last, boost::optional<HttpAbsoluteUrlPath> & result ) {
								parser::assert_not_empty( first, last );
								static auto const is_end_of_url = []( auto const & v ) { return parser::is_a( v, ' ' ); };

								using val_t = std::decay_t<std::remove_reference_t<decltype(*first)>>;
								auto url_bounds = parser::from_to( first, last, &parser::pred_true<val_t>, is_end_of_url, true );
								result.reset();
								HttpAbsoluteUrlPath url;
								auto bounds = path_parser( url_bounds.first, url_bounds.last, url.path );
								bounds = query_parser( bounds.last, url_bounds.last, url.query );
								fragment_parser( bounds.last, url_bounds.last, url.fragment );
								result = std::move( url );
								return url_bounds;
							}

						template<typename ForwardIterator>
							auto http_method_parser( ForwardIterator first, ForwardIterator last, HttpClientRequestMethod & result ) {
								parser::assert_not_empty( first, last );
								using val_t = std::decay_t<std::remove_reference_t<decltype(*first)>>;
								auto const method_bounds = parser::from_to( first, last, &parser::pred_true<val_t>, &parser::is_space<val_t>, true );
								result = http_request_method_from_string( method_bounds.as_string( ) );
								return method_bounds;
							};

						template<typename ForwardIterator>
							auto http_version_parser( ForwardIterator first, ForwardIterator last, std::string & result ) {
								// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
								using val_t = std::decay_t<std::remove_reference_t<decltype(*first)>>;
								parser::assert_not_empty( first, last );
								auto version_bounds = parser::from_to( first, last, parser::negate( &parser::is_space<val_t> ), &parser::pred_false<val_t> );
								parser::assert_not_empty( version_bounds.first, version_bounds.last );

								auto bounds = parser::from_to( version_bounds.first, version_bounds.last, &parser::is_number<val_t>, &parser::pred_false<val_t> );
								result = bounds.as_string( );
								return version_bounds;
							}

						template<typename ForwardIterator>
							auto request_line_parser( ForwardIterator first, ForwardIterator last, HttpRequestLine & result ) {
								parser::assert_not_empty( first, last );
								using val_t = std::decay_t<std::remove_reference_t<decltype(*first)>>;
								auto req_bounds = parser::until( first, last, parser::is_crlf<val_t>{ } );
								auto bounds = http_method_parser( req_bounds.first, req_bounds.last, result.method );
								boost::optional<HttpAbsoluteUrlPath> url;
								bounds = absolute_url_path_parser( bounds.last, req_bounds.last, url );
								if( !url ) {
									throw parser::ParserException{ };
								}
								result.url = *url;
								http_version_parser( bounds.last, req_bounds.last, result.version );
								return req_bounds;
							}

						template<typename ForwardIterator>
						    std::pair<std::string, std::string> header_pair_parser( ForwardIterator first, ForwardIterator last ) {
								static auto const not_token = parser::in( "()<>@,;:\\\"/[]?={} \x09" );
								static auto const is_token = parser::negate( not_token );

								// token >> : >> field_value >> crlf
								auto token_bounds = parser::from_to( first, last, is_token, not_token );

								parser::expect( *token_bounds.last, ':' );

							auto field_value_bounds = parser::until( std::next( token_bounds.last ), last, parser::is_crlf<decltype(*first)>{ } );

								return std::make_pair<std::string, std::string>( token_bounds.as_string( ), field_value_bounds.as_string( ) );
							}

						template<typename ForwardIterator>
						    auto header_parser( ForwardIterator first, ForwardIterator last, http::impl::HttpClientRequestImpl::headers_t & result ) {
								parser::assert_not_empty( first, last );
								auto header_bounds = parser::make_find_result( first, last - 2, true );
								using val_t = std::decay_t<std::remove_reference_t<decltype(*first)>>;
								auto headers = parser::split_if( first, last, parser::is_crlf<val_t>{ } );

								auto last_it = header_bounds.first;

								for( auto it : headers ) {
									auto cur_header = header_pair_parser( last_it, it );
									result[cur_header.first] = cur_header.second;
									last_it = std::next( it );
								}

								return header_bounds;
							}

						template<typename ForwardIterator>
						    auto request_parser( ForwardIterator first, ForwardIterator last, http::impl::HttpClientRequestImpl & result ) {
								using val_t = std::decay_t<std::remove_reference_t<decltype(*first)>>;
								auto req_bounds = parser::until( first, last, parser::is_crlf<val_t>{ 2 } );	// request ends after 2 crlf pairs
								parser::assert_not_empty( req_bounds.first, req_bounds.last );

								auto bounds = request_line_parser( req_bounds.first, req_bounds.last, result.request_line );
								header_parser( bounds.last, req_bounds.last, result.headers );

								return req_bounds;
							}

						template<typename ForwardIterator>
						    auto url_scheme_parser( ForwardIterator first, ForwardIterator last, std::string & result ) {
								static auto separator = parser::matcher( "://" );
								auto scheme_bounds = separator( first, last );

								parser::assert_not_equal( scheme_bounds.last, last );

								if( scheme_bounds ) {
									result = scheme_bounds.as_string( );
								}
								return scheme_bounds;
							}

						template<typename ForwardIterator>
						    auto url_auth_parser( ForwardIterator first, ForwardIterator last, boost::optional<UrlAuthInfo> & result ) {
								parser::assert_not_empty( first, last );
								auto auth_bounds = parser::until_value( first, last, '@' );
								if( !auth_bounds ) {
									result = boost::optional<UrlAuthInfo>{ };
									return auth_bounds;
								}
								auth_bounds.found = false;
								auto divider = parser::split_on( auth_bounds.first, auth_bounds.last, ':' );
								if( divider.size( ) != 2 ) {
									throw parser::ParserException{ };
								}
								auto username_bounds = parser::make_find_result( auth_bounds.first, divider[0] );
								auto password_bounds = parser::make_find_result( std::next( divider[0] ), divider[1] );
								result = UrlAuthInfo{ username_bounds.as_string( ), password_bounds.as_string( ) };
								auth_bounds.found = true;
								return auth_bounds;
							}

						template<typename ForwardIterator>
						    auto url_host_parser( ForwardIterator first, ForwardIterator last, std::string & result ) {
								static auto const not_valid = parser::in( "()<>@,;:\\\"/[]?={} \x09" );
								static auto const is_valid = parser::negate( not_valid );

								auto host_bounds = parser::from_to( first, last, is_valid, not_valid );
								result = host_bounds.as_string( );

								return host_bounds;
							}

						template<typename ForwardIterator, typename Result>
						    void to_uint( ForwardIterator first, ForwardIterator last, Result result ) {
								result = 0;
								for( ; first != last; ++first ) {
									result = (result * 10) + (*first - '0');
								}
							};

						template<typename ForwardIterator>
							auto url_port_parser( ForwardIterator first, ForwardIterator last, boost::optional<uint16_t> & result ) {
								if( !parser::is_a( *first, ':' ) ) {
									result = boost::optional<uint16_t>{ };
									return parser::make_find_result( first, last, false );
								}
								using value_t = daw::traits::root_type_t<decltype(*first)>;
								auto port_bounds = parser::from_to( std::next( first ), last, &parser::is_number<value_t>, parser::negate( &parser::is_number<value_t> ) );
								parser::assert_not_empty( port_bounds.first, port_bounds.last );
								to_uint( port_bounds.first, port_bounds.last, *result );
								return port_bounds;
							}

						template<typename ForwardIterator>
						    auto url_parser( ForwardIterator first, ForwardIterator last, http::impl::HttpUrlImpl & result ) {
								parser::assert_not_empty( first, last );
								auto bounds = url_scheme_parser( first, last, result.scheme );
								if( !bounds ) {
									throw parser::ParserException{ };
								}
								bounds = url_auth_parser( std::next( bounds.last, 3 )/*skip :// */, last, result.auth_info );
								bounds = url_host_parser( bounds.last, last, result.host );
								bounds = url_port_parser( bounds.last, last, result.port );
								bounds = absolute_url_path_parser( bounds.last, last, result.path );
								return parser::make_find_result( first, bounds.last, true );
							}
					}    // namespace impl

					template<typename ForwardIterator>
						auto http_absolute_url_path_parser( ForwardIterator first, ForwardIterator last ) {
							boost::optional<daw::nodepp::lib::http::HttpAbsoluteUrlPath> result;
							impl::absolute_url_path_parser( first, last, result );
							if( !result ) {
								throw parser::ParserException{ };
							}
							return *result;
						}

					template<typename ForwardIterator>
						auto http_request_parser( ForwardIterator first, ForwardIterator last ) {
							daw::nodepp::lib::http::impl::HttpClientRequestImpl result;
							impl::request_parser( first, last, result );
							return result;
						}

					template<typename ForwardIterator>
					    auto http_url_parser( ForwardIterator first, ForwardIterator last ) {
							daw::nodepp::lib::http::impl::HttpUrlImpl result;
							impl::url_parser( first, last, result );
							return result;
						}
				} // namespace http
			}	// namespace parse
		}    // namespace lib
	}    // namespace nodepp
}    // namespace daw

