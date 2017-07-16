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
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace parse {
					namespace impl {
						constexpr auto path_parser( daw::string_view str, std::string &result ) {
							parser::assert_not_empty( first, last );
							// char_( '/' )>> *(~char_( " ?#" ));
							struct is_first_t {
								template<typename Value>
								constexpr auto operator( )( Value const &v ) { return parser::is_a( v, '/' ); }
							};

							struct is_last_t {
								template<typename V>
								constexpr auto operator( )( Value const & v ) { return parser::is_a( v, ' ', '?', '#' ); }
							};

							auto bounds = parser::from_to( str.cbegin( ), str.cend( ), is_first_t{ }, is_last_t{ } );
							result = bounds.as_string( );
							return bounds;
						}

						auto parse_query_pair( daw::string_view str ) {
							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							auto const has_value_pos = daw::parser::find_first_of( str.cbegin( ), str.cend( ), '=' );

							HttpUrlQueryPair result;
							result.name = std::string{ str.cbegin( ), has_value_pos - str.cbegin( ) };
							if( has_value ) {
								result.value = std::string{ std::next( has_value_pos ), str.cend( ) };
							}
							return result;
						}

						auto parse_query_pairs( daw::string_view str ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							std::vector<HttpUrlQueryPair> pairs;
							{
								auto pos = std::find_first_of( str.cbegin( ), str.cend( ), '&' ); 
								auto last_pos = str.cbegin( );
								while( pos != str.cend( ) ) {
									pairs.push_back( parse_query_pair( last_pos, pos ) );
									last_pos = pos;
									pos = std::find_first_of( std::next( pos ), str.cend( ), '&' );
								}
								if( pos != str.cend( ) ) {
									pairs.push_back( parse_query_pair( last_pos, pos ) );
								}
							}
							return pairs;
						}

						auto query_parser( daw::string_view str, std::vector<HttpUrlQueryPair> &result ) {
							// lit( '?' )>> query_pair>> *((qi::lit( ';' ) | '&')>> query_pair);
							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							struct is_first_t {
								template<typename Value>
								constexpr auto operator( )( Value const &v ) { return parser::is_a( v, '?' ); }
							};

							struct is_last_t {
								template<typename V>
								constexpr auto operator( )( Value const & v ) { return parser::is_a( v, '#', ' ' ); }
							};

							auto bounds = parser::from_to( first, last, is_first_t{ }, is_last_t{ } );
							result = parse_query_pairs( bounds.first, bounds.last );
							return bounds;
						}

						auto fragment_parser( daw::string_view str, boost::optional<std::string> &result ) {
							// lit( '#' )>> *(~char_( " " ));
							auto bounds = parser::from_to( first, last, '#', ' ' );
							if( !bounds.empty( ) ) {
								result = bounds.as_string( );
							}
							return bounds;
						}

						auto absolute_url_path_parser( daw::string_view str,
						                               boost::optional<HttpAbsoluteUrlPath> &result ) {

							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							struct is_end_of_url_t {
								template<typename Value>
								constexpr auto operator( )( Value const &v ) { return parser::is_a( v, ' ' ); }
							};

							using val_t = typename daw::string_view::value_type;
							auto url_bounds = parser::from_to( str.cbegin( ), str.cend( ), &parser::pred_true<val_t>,
							                                   is_end_of_url_t{}, true );
							result.reset( );
							HttpAbsoluteUrlPath url;
							auto bounds = path_parser( url_bounds.first, url_bounds.last, url.path );
							bounds = query_parser( bounds.last, url_bounds.last, url.query );
							fragment_parser( bounds.last, url_bounds.last, url.fragment );
							result = std::move( url );
							return url_bounds;
						}

						constexpr auto http_method_parser( daw::string_view str, HttpClientRequestMethod &result ) {
							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							using val_t = typename daw::string_view::value_type;
							auto method_bounds = parser::from_to( first, last, &parser::pred_true<val_t>,
							                                            &parser::is_space<val_t>, true );
							result = http_request_method_from_string( method_bounds.as_string( ) );
							return method_bounds;
						};

						constexpr auto http_version_parser( daw::string_view str, daw::string_view &result ) {
							// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							using val_t = typename daw::string_view::value_type;
							auto version_bounds = parser::from_to(
							    str.cbegin( ), str.cend( ), parser::negate( &parser::is_space<val_t> ), &parser::pred_false<val_t> );

							parser::assert_not_empty( version_bounds.first, version_bounds.last );

							auto bounds = parser::from_to( version_bounds.first, version_bounds.last,
							                               &parser::is_number<val_t>, &parser::pred_false<val_t> );
							result = daw::make_string_view_it( bounds.first, bounds.last );
							return version_bounds;
						}

						auto request_line_parser( daw::string_view str, HttpRequestLine &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto new_line_pos = parser::find_first_of_when( str, parser::is_crlf<char>{ } );

							auto bounds = http_method_parser(
							    daw::make_string_view_it( str.cbegin( ), new_line_pos ), result.method );

							boost::optional<HttpAbsoluteUrlPath> url;
							bounds = absolute_url_path_parser( daw::make_string_view_it( bounds.last, new_line_pos ),
							                                   url );
							if( !url ) {
								throw parser::ParserException{};
							}
							result.url = *url;
							http_version_parser( daw::make_string_view_it( bounds.last, new_line_pos ),
							                     result.version );
							return req_bounds;
						}

						std::pair<std::string, std::string> header_pair_parser( daw::string_view str ) {
							struct tokens_t {
								static auto const not_token = parser::in( "()<>@,;:\\\"/[]?={} \x09" );
								static auto const is_token = parser::negate( not_token );
							};

							// token >> : >> field_value >> crlf
							auto token_bounds = parser::from_to( str.cbegin( ), str.cend( ), is_token, not_token );

							parser::expect( *token_bounds.last, ':' );

							auto field_value_bounds =
							    parser::until( std::next( token_bounds.last ), str.cend( ),
							                   parser::is_crlf<typename daw::string_view::value_type>{} );

							return std::make_pair<std::string, std::string>( token_bounds.as_string( ),
							                                                 field_value_bounds.as_string( ) );
						}

						constexpr auto header_parser( daw::string_view str,
						                    http::impl::HttpClientRequestImpl::headers_t &result ) {

							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							auto header_bounds = parser::make_find_result( str.cbegin( ), str.cend( ) - 2, true );

							using val_t = typename daw::string_view::value_type;
							auto headers = parser::split_if( str.cbegin( ), str.cend( ), parser::is_crlf<val_t>{} );

							auto last_it = header_bounds.first;

							for( auto it : headers ) {
								auto cur_header = header_pair_parser( last_it, it );
								result[cur_header.first] = cur_header.second;
								last_it = std::next( it );
							}

							return header_bounds;
						}

						constexpr auto request_parser( daw::string_view str, http::impl::HttpClientRequestImpl &result ) {
							using val_t = typename daw::string_view::value_type;

							auto req_bounds =
							    parser::until( str.cbegin( ), str.cend( ),
							                   parser::is_crlf<val_t>{2} ); // request ends after 2 crlf pairs
							parser::assert_not_empty( req_bounds.first, req_bounds.last );

							auto bounds = request_line_parser( req_bounds.first, req_bounds.last, result.request_line );
							header_parser( bounds.last, req_bounds.last, result.headers );

							return req_bounds;
						}

						auto url_scheme_parser( daw::string_view str, daw::string_view &result ) {
							static auto separator = parser::matcher( "://" );
							auto scheme_bounds = separator( first, last );

							parser::assert_not_equal( scheme_bounds.last, last );

							if( scheme_bounds ) {
								result = daw::make_string_view_it( scheme_bounds.first, scheme_bounds.last );
							}
							return scheme_bounds;
						}

						auto url_auth_parser( daw::string_view str, boost::optional<UrlAuthInfo> &result ) {
							parser::assert_not_empty( str.cbegin( ), str.cend( ) );
							auto auth_bounds = parser::until_value( str.cbegin( ), str.cend( ), '@' );
							if( !auth_bounds ) {
								result = boost::optional<UrlAuthInfo>{};
								return auth_bounds;
							}
							auth_bounds.found = false;
							auto divider = parser::split_on( auth_bounds.first, auth_bounds.last, ':' );
							if( divider.size( ) != 2 ) {
								throw parser::ParserException{};
							}
							auto username_bounds = parser::make_find_result( auth_bounds.first, divider[0] );
							auto password_bounds = parser::make_find_result( std::next( divider[0] ), divider[1] );
							result = UrlAuthInfo{username_bounds.as_string( ), password_bounds.as_string( )};
							auth_bounds.found = true;
							return auth_bounds;
						}

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

						template<typename ForwardIterator>
						auto url_parser( ForwardIterator first, ForwardIterator last,
						                 http::impl::HttpUrlImpl &result ) {
							parser::assert_not_empty( first, last );
							auto bounds = url_scheme_parser( first, last, result.scheme );
							if( !bounds ) {
								throw parser::ParserException{};
							}
							bounds =
							    url_auth_parser( std::next( bounds.last, 3 ) /*skip :// */, last, result.auth_info );
							bounds = url_host_parser( bounds.last, last, result.host );
							bounds = url_port_parser( bounds.last, last, result.port );
							bounds = absolute_url_path_parser( bounds.last, last, result.path );
							return parser::make_find_result( first, bounds.last, true );
						}
					} // namespace impl

					template<typename ForwardIterator>
					auto http_absolute_url_path_parser( ForwardIterator first, ForwardIterator last ) {
						boost::optional<daw::nodepp::lib::http::HttpAbsoluteUrlPath> result;
						impl::absolute_url_path_parser( first, last, result );
						if( !result ) {
							throw parser::ParserException{};
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
				} // namespace parse
			}     // namespace http
		}         // namespace lib
	}             // namespace nodepp
} // namespace daw
