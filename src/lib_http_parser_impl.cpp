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

#include <string>

#include <daw/daw_string_view.h>
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
						struct is_a_space_t {
							constexpr auto operator( )( char const v ) noexcept {
								return ' ' == v;
							}
						}; // is_a_space_t

						struct is_not_a_space_t {
							constexpr auto operator( )( char const v ) noexcept {
								return ' ' != v;
							}
						}; // is_not_a_space_t

						daw::parser::find_result_t<char const *> path_parser( daw::string_view str,
						                                                      std::string &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							// char_( '/' )>> *(~char_( " ?#" ));

							struct {
								constexpr auto operator( )( char const v ) noexcept {
									return parser::is_a( v, ' ', '?', '#' );
								}
							} const is_last{ };

							auto first_pos = str.find_first_of( '/' );
							if( first_pos == str.npos ) {
								return daw::parser::make_find_result( str.cbegin( ), str.cend( ) );
							}
							auto first = str.cbegin( ) + first_pos;

							auto last = std::find_if( first, str.cend( ), is_last );
							auto const sz = std::distance( first, last );
							if( sz == 0 ) {
								return daw::parser::make_find_result( str.cbegin( ), str.cend( ) );
							}

							result = std::string( first, sz );

							auto bounds = daw::parser::make_find_result( first, last, true );

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
							if( str.empty( ) ) {
								return daw::parser::make_find_result( str.cbegin( ), str.cend( ) );
							}

							struct {
								constexpr auto operator( )( char const v ) noexcept {
									return v == '?';
								}
							} const is_first{};

							struct {
								constexpr auto operator( )( char const v ) noexcept {
									return parser::is_a( v, '#', ' ' );
								}
							} const is_last{};

							auto bounds = parser::from_to_pred( str.cbegin( ), str.cend( ), is_first, is_last );
							result = parse_query_pairs( daw::make_string_view_it( bounds.first, bounds.last ) );
							return bounds;
						}

						daw::parser::find_result_t<char const *>
						fragment_parser( daw::string_view str, boost::optional<std::string> &result ) {
							// lit( '#' )>> *(~char_( " " ));
							if( str.empty( ) ) {
								return daw::parser::make_find_result( str.cbegin( ), str.cend( ) );
							}
							auto bounds = parser::from_to( str.cbegin( ), str.cend( ), '#', ' ' );
							if( !bounds.empty( ) ) {
								result = bounds.to_string( );
							}
							return bounds;
						}


						daw::parser::find_result_t<char const *>
						absolute_url_path_parser( daw::string_view str, boost::optional<HttpAbsoluteUrlPath> &result ) {

							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto url_bounds = parser::from_to_pred(
							    str.cbegin( ), str.cend( ), impl::is_not_a_space_t{ }, impl::is_a_space_t{ }, true );
							result.reset( );
							HttpAbsoluteUrlPath url;
							auto bounds = path_parser( daw::make_string_view_it( url_bounds.first, url_bounds.last ), url.path );
							bounds = query_parser( daw::make_string_view_it( bounds.last, url_bounds.last ), url.query );
							fragment_parser( daw::make_string_view_it( bounds.last, url_bounds.last ), url.fragment );
							result = std::move( url );
							return url_bounds;
						}

						daw::parser::find_result_t<char const *> http_version_parser( daw::string_view str,
						                                                              std::string &result ) {
							// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							using val_t = typename daw::string_view::value_type;
							auto version_bounds =
							    parser::from_to_pred( str.cbegin( ), str.cend( ), impl::is_not_a_space_t{ },
							                     &parser::is_unicode_whitespace<val_t> );

							auto bounds = parser::from_to_pred( version_bounds.first, version_bounds.last,
							                               &parser::is_number<val_t>, &parser::pred_false<val_t> );
							result = bounds.to_string_view( );
							return version_bounds;
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

						constexpr auto skip_newline( daw::string_view str ) noexcept {
							if( str.front( ) == '\r' ) {
								str.remove_prefix( );
							}
							if( str.front( ) == '\n' ) {
								str.remove_prefix( );
							}
							return str;
						}

						std::pair<daw::string_view, daw::string_view> header_pair_parser( daw::string_view str ) {
							// token >> : >> field_value
							auto name_end_pos = std::find( str.cbegin( ), str.cend( ), ':' );

							daw::exception::daw_throw_on_false( name_end_pos != str.cend( ), "Expected a : to divide header" );

							auto value_start_pos = std::next( name_end_pos );
							while( value_start_pos != str.cend( ) && daw::parser::is_unicode_whitespace( *value_start_pos ) ) {
								++value_start_pos;
							}
							daw::exception::daw_throw_on_false( value_start_pos != str.cend( ), "Could not find start of value" );

							auto name = daw::make_string_view_it( str.cbegin( ), name_end_pos );
							auto value = daw::make_string_view_it( value_start_pos, str.cend( ) );

							return {std::move( name ), std::move( value )};
						}

						daw::parser::find_result_t<char const *> url_scheme_parser( daw::string_view str,
						                                                            daw::string_view result ) {
							static auto const separator = parser::matcher( "://" );
							auto const scheme_bounds = separator( str.cbegin( ), str.cend( ) );

							parser::assert_not_equal( scheme_bounds.last, str.cend( ) );

							if( scheme_bounds ) {
								result = scheme_bounds.to_string_view( );
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
							result = UrlAuthInfo{username_bounds.to_string( ), password_bounds.to_string( )};
							auth_bounds.found = true;
							return auth_bounds;
						}

						daw::parser::find_result_t<char const *>
						http_method_parser( daw::string_view str,
						                    daw::nodepp::lib::http::HttpClientRequestMethod &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto method_bounds = parser::from_to_pred( str.cbegin( ), str.cend( ), &parser::pred_true<char>,
							                                      impl::is_a_space_t{ }, true );
							result = http_request_method_from_string( method_bounds.to_string_view( ) );
							return method_bounds;
						}

						std::vector<daw::string_view> split_headers( daw::string_view str ) {
							std::vector<daw::string_view> result;
							auto pos = str.find( '\n' );
							while( !str.empty( ) && pos != str.npos ) {
								daw::string_view tmp( str, pos-1 );
								if( !tmp.empty( ) && tmp.back( ) == '\r' ) {
									tmp.remove_suffix( );
								}
								if( !tmp.empty( ) ) {
									result.push_back( std::move( tmp ) );
								}
								str.remove_prefix( pos + 1 );
								pos = str.find( '\n' );
							}
							return result;
						}

						daw::parser::find_result_t<char const *>
						header_parser( daw::string_view str, http::impl::HttpClientRequestImpl::headers_t &result ) {

							if( str.empty( ) ) {
								return daw::parser::make_find_result( str.cbegin( ), str.cend( ) );
							}
							auto header_bounds = parser::make_find_result( str.cbegin( ), str.cend( ) - 2, true );

							auto const headers = split_headers( str );

							for( auto const header : headers ) {
								auto cur_header = header_pair_parser( header );
								auto name = cur_header.first.to_string( );
								auto value = cur_header.second.to_string( );
								result.add( std::move( name ), std::move( value ) );
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
							    url_auth_parser( daw::make_string_view_it( std::next( bounds.last, 3 ), str.cend( ) ),
							                     result.auth_info );
							bounds =
							    url_host_parser( daw::make_string_view_it( bounds.last, str.cend( ) ), result.host );

							bounds =
							    url_port_parser( daw::make_string_view_it( bounds.last, str.cend( ) ), result.port );

							bounds = absolute_url_path_parser( daw::make_string_view_it( bounds.last, str.cend( ) ),
							                                   result.path );
							return parser::make_find_result( str.cbegin( ), bounds.last, true );
						}

					} // namespace impl
				}     // namespace parse
			}         // namespace http
		}             // namespace lib
	}                 // namespace nodepp
} // namespace daw
