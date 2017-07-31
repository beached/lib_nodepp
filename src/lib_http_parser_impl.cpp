// The MIT License (MIT)
//
// Copyright (c) 2017 Darrell Wright
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

#include <daw/daw_parser_helper.h>

#include "lib_http_parser_impl.h"
#include "lib_http_request.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace parse {
					namespace impl {
						daw::parser::find_result_t<char const *> path_parser( daw::string_view str,
						                                                      std::string &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							// char_( '/' )>> *(~char_( " ?#" ));
							struct is_first_t {
								constexpr auto operator( )( char v ) {
									return parser::is_a( v, '/' );
								}
							};

							struct is_last_t {
								constexpr auto operator( )( char v ) {
									return parser::is_a( v, ' ', '?', '#' );
								}
							};

							auto bounds = parser::from_to( str.cbegin( ), str.cend( ), is_first_t{}, is_last_t{} );
							result = bounds.as_string( );
							return bounds;
						}

						daw::nodepp::lib::http::HttpUrlQueryPair parse_query_pair( daw::string_view str ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto const has_value_pos = daw::parser::find_first_of( str, '=' );

							daw::nodepp::lib::http::HttpUrlQueryPair result;
							result.name =
							    std::string{str.cbegin( ), static_cast<size_t>( has_value_pos - str.cbegin( ) )};
							if( has_value_pos != str.cend( ) ) {
								result.value = std::string{std::next( has_value_pos ), str.cend( )};
							}
							return result;
						}

						std::vector<daw::nodepp::lib::http::HttpUrlQueryPair>
						parse_query_pairs( daw::string_view str ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							std::vector<daw::nodepp::lib::http::HttpUrlQueryPair> pairs;
							{
								auto pos = daw::parser::find_first_of( str, '&' );
								auto last_pos = str.cbegin( );
								while( pos != str.cend( ) ) {
									pairs.push_back( parse_query_pair( daw::make_string_view_it( last_pos, pos ) ) );
									last_pos = pos;
									pos = daw::parser::find_first_of(
									    daw::make_string_view_it( std::next( pos ), str.cend( ) ), '&' );
								}
								if( pos != str.cend( ) ) {
									pairs.push_back( parse_query_pair( daw::make_string_view_it( last_pos, pos ) ) );
								}
							}
							return pairs;
						}

						daw::parser::find_result_t<char const *> query_parser( daw::string_view str,
						                                                       std::vector<HttpUrlQueryPair> &result ) {
							// lit( '?' )>> query_pair>> *((qi::lit( ';' ) | '&')>> query_pair);
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							struct is_first_t {
								constexpr auto operator( )( char v ) {
									return parser::is_a( v, '?' );
								}
							};

							struct is_last_t {
								constexpr auto operator( )( char v ) {
									return parser::is_a( v, '#', ' ' );
								}
							};

							auto bounds = parser::from_to( str.cbegin( ), str.cend( ), is_first_t{}, is_last_t{} );
							result = parse_query_pairs( daw::make_string_view_it( bounds.first, bounds.last ) );
							return bounds;
						}

						daw::parser::find_result_t<char const *>
						fragment_parser( daw::string_view str, boost::optional<std::string> &result ) {
							// lit( '#' )>> *(~char_( " " ));
							auto bounds = parser::from_to( str.cbegin( ), str.cend( ), '#', ' ' );
							if( !bounds.empty( ) ) {
								result = bounds.as_string( );
							}
							return bounds;
						}

						daw::parser::find_result_t<char const *>
						absolute_url_path_parser( daw::string_view str, boost::optional<HttpAbsoluteUrlPath> &result ) {

							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							struct is_end_of_url_t {
								constexpr auto operator( )( char v ) {
									return parser::is_a( v, ' ' );
								}
							};

							using val_t = typename daw::string_view::value_type;
							auto url_bounds = parser::from_to( str.cbegin( ), str.cend( ), &parser::pred_true<val_t>,
							                                   is_end_of_url_t{}, true );
							result.reset( );
							HttpAbsoluteUrlPath url;
							auto bounds = path_parser( daw::make_string_view_it( url_bounds.first, url_bounds.last ), url.path );
							bounds = query_parser( daw::make_string_view_it( bounds.last, url_bounds.last ), url.query );
							fragment_parser( daw::make_string_view_it( bounds.last, url_bounds.last ), url.fragment );
							result = std::move( url );
							return url_bounds;
						}

						daw::parser::find_result_t<char const *> request_line_parser( daw::string_view str,
						                                                              HttpRequestLine &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto new_line_pos = parser::find_first_of_when( str, parser::is_crlf<char>{} );

							auto bounds = http_method_parser( daw::make_string_view_it( str.cbegin( ), new_line_pos ),
							                                  result.method );

							boost::optional<HttpAbsoluteUrlPath> url;
							bounds =
							    absolute_url_path_parser( daw::make_string_view_it( bounds.last, new_line_pos ), url );
							if( !url ) {
								throw parser::ParserException{};
							}
							result.url = *url;
							return http_version_parser( daw::make_string_view_it( bounds.last, new_line_pos ),
							                            result.version );
						}

						std::pair<std::string, std::string> header_pair_parser( daw::string_view str ) {
							struct tokens_t {
								auto const not_token = parser::in( "()<>@,;:\\\"/[]?={} \x09" );
								auto const is_token = parser::negate( not_token );
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

						daw::parser::find_result_t<char const *> url_scheme_parser( daw::string_view str,
						                                                            daw::string_view result ) {
							static auto const separator = parser::matcher( "://" );
							auto const scheme_bounds = separator( first, last );

							parser::assert_not_equal( scheme_bounds.last, last );

							if( scheme_bounds ) {
								result = scheme.to_string_view( );
							}
							return scheme_bounds;
						}

						daw::parser::find_result_t<char const *>
						url_auth_parser( daw::string_view str, boost::optional<UrlAuthInfo> &result ) {

							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
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

						daw::parser::find_result_t<char const *>
						http_method_parser( daw::string_view str,
						                    daw::nodepp::lib::http::HttpClientRequestMethod &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto method_bounds = parser::from_to( str.cbegin( ), str.cend( ), &parser::pred_true<char>,
							                                      &parser::is_space<char>, true );
							result = http_request_method_from_string( method_bounds.to_string_view( ) );
							return method_bounds;
						}

						daw::parser::find_result_t<char const *> http_version_parser( daw::string_view str,
						                                                              daw::string_view result ) {
							// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							using val_t = typename daw::string_view::value_type;
							auto version_bounds =
							    parser::from_to( str.cbegin( ), str.cend( ), parser::negate( &parser::is_space<val_t> ),
							                     &parser::pred_false<val_t> );

							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto bounds = parser::from_to( version_bounds.first, version_bounds.last,
							                               &parser::is_number<val_t>, &parser::pred_false<val_t> );
							result = bounds.to_string_view( );
							return version_bounds;
						}

						daw::parser::find_result_t<char const *>
						header_parser( daw::string_view str, http::impl::HttpClientRequestImpl::headers_t &result ) {

							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
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

						daw::parser::find_result_t<char const *>
						request_parser( daw::string_view str, http::impl::HttpClientRequestImpl &result ) {
							using val_t = typename daw::string_view::value_type;

							auto req_bounds =
							    parser::until( str.cbegin( ), str.cend( ),
							                   parser::is_crlf<val_t>{2} ); // request ends after 2 crlf pairs
							parser::assert_not_empty( req_bounds.first, req_bounds.last );

							auto bounds = request_line_parser( req_bounds.to_string_view( ), result.request_line );
							header_parser( daw::make_string_view_it( bounds.last, req_bounds.last ), result.headers );

							return req_bounds;
						}

						daw::parser::find_result_t<char const *> url_parser( daw::string_view str,
						                                                     http::impl::HttpUrlImpl &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto bounds = url_scheme_parser( str, result.scheme );
							if( !bounds ) {
								throw parser::ParserException{};
							}
							bounds =
							    url_auth_parser( std::next( bounds.last, 3 ) /*skip :// */, str.cend( ), result.auth_info );
							bounds = url_host_parser( bounds.last, str.cend( ), result.host );
							bounds = url_port_parser( bounds.last, str.cend( ), result.port );
							bounds = absolute_url_path_parser( bounds.last, str.cend( ), result.path );
							return parser::make_find_result( first, bounds.last, true );
						}

					} // namespace impl
				}     // namespace parse
			}         // namespace http
		}             // namespace lib
	}                 // namespace nodepp
} // namespace daw
