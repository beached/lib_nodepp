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

#include <daw/daw_container_algorithm.h>
#include <daw/daw_function_iterator.h>
#include <daw/daw_parser_helper.h>
#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_string_view.h>

#include "lib_http_parser_impl.h"
#include "lib_http_request.h"
#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace parse {
					namespace impl {
						daw::string_view path_parser( daw::string_view str, std::string &result ) {

							// char_( '/' )>> *(~char_( " ?#" ));
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							daw::exception::daw_throw_on_false( str.front( ) == '/', "Invalid path" );

							str = str.substr( 0, str.find_first_of( " ?#" ) );
							daw::exception::daw_throw_on_true( str.empty( ), "Invalid path to parse" );
							result = url_decode( str );
							return str;
						}

						daw::nodepp::lib::http::HttpUrlQueryPair parse_query_pair( daw::string_view str ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto const has_value_pos = str.find_first_of( '=' );

							daw::nodepp::lib::http::HttpUrlQueryPair result;
							result.name = url_decode( str.substr( 0, has_value_pos ) );

							str.remove_prefix( has_value_pos + 1 ); // account for = symbol
							if( !str.empty( ) ) {
								result.value = url_decode( str );
							}
							return result;
						}

						std::vector<daw::nodepp::lib::http::HttpUrlQueryPair> parse_query_pairs( daw::string_view str ) {
							// FIXME, need to account for everything
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							std::vector<daw::nodepp::lib::http::HttpUrlQueryPair> pairs;
							while( !str.empty( ) ) {
								auto cur_len = str.find_first_of( '&' );
								auto qp = parse_query_pair( str.substr( 0, cur_len ) );
								pairs.push_back( std::move( qp ) );
								if( cur_len >= str.size( ) ) {
									cur_len = str.size( ) - 1;
								}
								str = str.substr( cur_len + 1 );
							}
							return pairs;
						}

						daw::string_view query_parser( daw::string_view str, std::vector<HttpUrlQueryPair> &result ) {
							// lit( '?' )>> query_pair>> *((qi::lit( ';' ) | '&')>> query_pair);
							if( str.empty( ) || str.front( ) != '?' ) {
								return str;
							}
							str.remove_prefix( );
							if( str.empty( ) ) {
								return str;
							}

							str = str.substr( 0, str.find_first_of( "# " ) );

							if( !str.empty( ) ) {
								result = parse_query_pairs( str );
							}

							return str;
						}

						daw::string_view fragment_parser( daw::string_view str, boost::optional<std::string> &result ) {
							// lit( '#' )>> *(~char_( " " ));
							if( str.empty( ) || str.front( ) != '#' ) {
								result = boost::none;
								return str;
							}
							str.remove_prefix( );
							str = str.substr( 0, str.find_first_of( ' ' ) );

							result = url_decode( str );
							return str;
						}

						daw::string_view absolute_url_path_parser( daw::string_view str,
						                                           boost::optional<HttpAbsoluteUrlPath> &result ) {
							str = str.substr( 0, str.find_first_of( ' ' ) );
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							result.reset( );
							HttpAbsoluteUrlPath url;
							auto const first = str.cbegin( );
							auto const path_str = path_parser( str, url.path );

							str = daw::parser::trim_left( daw::make_string_view_it( path_str.cend( ), str.cend( ) ) );

							auto const query_bounds = query_parser( str, url.query );

							str = daw::parser::trim_left( daw::make_string_view_it( query_bounds.cend( ), str.cend( ) ) );
							fragment_parser( str, url.fragment );

							str = daw::parser::trim_left( daw::make_string_view_it( query_bounds.cend( ), str.cend( ) ) );

							result = std::move( url );
							return daw::make_string_view_it( first, str.cend( ) );
						}

						daw::string_view http_version_parser( daw::string_view str, std::string &result ) {
							// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto version_pos = str.search( "HTTP/" );
							daw::exception::daw_throw_on_false( version_pos < str.size( ), "Invalid HTTP Version" );
							str = str.substr( version_pos + 5 );

							daw::exception::daw_throw_on_true( str.empty( ), "Invalid HTTP Version" );

							auto const is_valid_number =
							  str.find_first_not_of_if( []( auto c ) { return ( c == '.' ) || ( '0' <= c && c <= '9' ); } );

							daw::exception::daw_throw_on_false( is_valid_number == str.npos, "Invalid HTTP version" + str );

							result = str.to_string( );
							return str;
						}

						constexpr daw::string_view http_method_parser( daw::string_view str,
						                                               daw::nodepp::lib::http::HttpClientRequestMethod &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							str = str.substr( 0, str.find_first_of( ' ' ) );

							result = http_request_method_from_string( str );

							return str;
						}

						daw::string_view request_line_parser( daw::string_view str, HttpRequestLine &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto bounds = http_method_parser( str, result.method );

							boost::optional<HttpAbsoluteUrlPath> url;

							str = daw::parser::trim_left( daw::make_string_view_it( bounds.cend( ), str.cend( ) ) );

							bounds = absolute_url_path_parser( str, url ); // account for space after method

							daw::exception::daw_throw_on_false( url, "Invalid url format" );

							result.url = *url;
							str = daw::parser::trim_left( daw::make_string_view_it( bounds.cend( ), str.cend( ) ) );

							return http_version_parser( str, result.version );
						}

						std::pair<daw::string_view, daw::string_view> header_pair_parser( daw::string_view str ) {
							// token >> : >> field_value
							auto const name_end_pos = str.find_first_of( ':' );
							daw::exception::daw_throw_on_false( name_end_pos < str.size( ), "Expected a : to divide header" );

							auto name = str.substr( 0, name_end_pos );
							auto value = daw::parser::trim_left( str.substr( name_end_pos + 1 ) );

							return {name, value};
						}

						daw::string_view url_scheme_parser( daw::string_view str, std::string &scheme ) {
							auto const scheme_bounds_pos = str.search( "://" );
							daw::exception::daw_throw_on_false( scheme_bounds_pos < str.size( ), "Could not find end of url scheme" );

							str = str.substr( 0, scheme_bounds_pos );
							scheme = url_decode( str );
							return str;
						}

						daw::string_view url_auth_parser( daw::string_view str, boost::optional<UrlAuthInfo> &result ) {
							// username:password@
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							auto const auth_end_pos = str.find_last_of( '@' );
							if( auth_end_pos > str.size( ) ) {
								result = boost::none;
								return str;
							}
							str = str.substr( auth_end_pos );

							auto const divider_pos = str.find_first_of( ':' );
							daw::exception::daw_throw_on_false( divider_pos < str.size( ) );

							auto const username = str.substr( 0, divider_pos );
							auto const password = str.substr( divider_pos + 1 );
							result = UrlAuthInfo{url_decode( username ), url_decode( password )};
							return str;
						}

						std::vector<daw::string_view> split_headers( daw::string_view str ) {
							std::vector<daw::string_view> headers;
							auto pos = str.search( "\r\n" );

							while( !str.empty( ) ) {
								auto tmp = str.substr( 0, pos );
								if( !tmp.empty( ) ) {
									headers.push_back( tmp );
								}
								if( pos < str.size( ) ) {
									str.remove_prefix( pos + 2 );
									pos = str.search( "\r\n" );
								} else {
									str.remove_prefix( str.size( ) );
								}
							}

							return headers;
						}

						daw::string_view header_parser( daw::string_view str,
						                                http::impl::HttpClientRequestImpl::headers_t &result ) {
							str = daw::parser::trim_left( str.substr( 0, str.search( "\r\n\r\n" ) ) );
							auto str_result = str;
							if( str.empty( ) ) {
								return str;
							}
							str_result.resize( str.size( ) + 4 );

							daw::container::transform(
							  split_headers( str ),
							  daw::make_function_iterator( [&result]( auto val ) { result.add( std::move( val ) ); } ),
							  []( auto const &header ) {
								  auto const cur_header = header_pair_parser( header );
								  return http::HttpClientRequestHeader{cur_header.first.to_string( ), cur_header.second.to_string( )};
							  } );

							return str_result;
						}

						daw::string_view request_parser( daw::string_view str, http::impl::HttpClientRequestImpl &result ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto const req_end_pos = str.search( "\r\n" );
							daw::exception::daw_throw_on_false( req_end_pos < str.size( ),
							                                    "Invalid request, does not end in newline" );

							auto const req_bounds = request_line_parser( str.substr( 0, req_end_pos ), result.request_line );
							auto const head_bounds = header_parser( str.substr( req_end_pos + 2 ),
							                                        result.headers ); // accounts for size of "\r\n"

							return daw::make_string_view_it( req_bounds.cbegin( ), head_bounds.cend( ) );
						}

						daw::string_view url_host_parser( daw::string_view str, std::string &result ) {
							static daw::string_view const invalid_vals = R"(()<>@,;:\"/[]?={} \x09)";

							auto const first_pos = str.find_first_of( invalid_vals );
							daw::exception::daw_throw_on_false( first_pos < str.size( ) );
							str = str.substr( first_pos );
							str = str.substr( 0, str.find_first_of( invalid_vals ) );
							result = url_decode( str );
							return str;
						}

						daw::string_view url_port_parser( daw::string_view str, boost::optional<uint16_t> &result ) {
							if( str.empty( ) || str.front( ) != ':' ) {
								result = boost::none;
								return str;
							}
							str.remove_prefix( );
							str = daw::parser::trim_left( str );
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected port number format" );

							str = str.substr( 0, str.find_first_not_of_if( &parser::is_number<char> ) );
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected port number format" );
							parser::parse_unsigned_int( str.cbegin( ), str.cend( ), *result );
							return str;
						}

						daw::string_view url_parser( daw::string_view str, http::impl::HttpUrlImpl &result ) {

							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							auto bounds = url_scheme_parser( str, result.scheme );

							bounds = url_auth_parser( daw::make_string_view_it( std::next( bounds.cend( ), 3 ), str.cend( ) ),
							                          result.auth_info ); // account for "://"
							bounds = url_host_parser( daw::make_string_view_it( bounds.cend( ), str.cend( ) ), result.host );

							bounds = url_port_parser( daw::make_string_view_it( bounds.cend( ), str.cend( ) ), result.port );

							bounds = absolute_url_path_parser( daw::make_string_view_it( bounds.cend( ), str.cend( ) ), result.path );

							return daw::make_string_view_it( str.cbegin( ), bounds.cend( ) );
						}
					} // namespace impl

					daw::nodepp::lib::http::HttpAbsoluteUrlPath http_absolute_url_path_parser( daw::string_view str ) {
						boost::optional<daw::nodepp::lib::http::HttpAbsoluteUrlPath> result;
						impl::absolute_url_path_parser( str, result );
						daw::exception::daw_throw_on_false( result, "Error parsing absolute URL" );
						return *result;
					}

					daw::nodepp::lib::http::impl::HttpClientRequestImpl http_request_parser( daw::string_view str ) {
						daw::nodepp::lib::http::impl::HttpClientRequestImpl result;
						impl::request_parser( str, result );
						return result;
					}

					daw::nodepp::lib::http::impl::HttpUrlImpl http_url_parser( daw::string_view str ) {
						daw::nodepp::lib::http::impl::HttpUrlImpl result;
						impl::url_parser( str, result );
						return result;
					}
				} // namespace parse
			}   // namespace http
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
