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

#include <cctype>
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
						std::string path_parser( daw::string_view str ) {
							// starts with '/' and ends with a ' ', '?', or '#'
							daw::exception::daw_throw_on_true( str.empty( ) || str.front( ) != '/', "Invalid path" );
							return url_decode( str );
						}

						HttpUrlQueryPair parse_query_pair( daw::string_view str ) {
							HttpUrlQueryPair result{};
							result.name = url_decode( str.pop_front( "=" ) );
							if( !str.empty( ) ) {
								result.value = url_decode( str );
							}
							return result;
						}

						std::vector<HttpUrlQueryPair> query_parser( daw::string_view str ) {
							// FIXME, need to account for everything
							std::vector<HttpUrlQueryPair> pairs{};
							while( !str.empty( ) ) {
								pairs.push_back( parse_query_pair( str.pop_front( "&" ) ) );
							}
							return pairs;
						}

						boost::optional<std::string> fragment_parser( daw::string_view str ) {
							if( str.empty( ) ) {
								return boost::none;
							}
							return url_decode( str );
						}

						HttpAbsoluteUrlPath absolute_url_path_parser( daw::string_view str ) {
							// Find " " preceeding HTTP/1.1
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );
							daw::exception::daw_throw_on_false( str.front( ) == '/', "Unexpected path format" );

							HttpAbsoluteUrlPath url{};
							{
								auto fragment = str.pop_back( "#" );
								if( str.empty( ) ) {
									str = fragment;
								} else {
									url.fragment = fragment_parser( fragment );
								}
							}

							url.path = path_parser( str.pop_front( "?" ) );
							url.query = query_parser( str );
							return url;
						}

						daw::string_view http_version_parser( daw::string_view str, std::string &result ) {
							std::cout << "http_version_parser: '" << str << "'\n";
							// lexeme["HTTP/">> raw[int_>> '.'>> int_]]
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							str.pop_front( "HTTP/" );
							daw::exception::daw_throw_on_true( str.empty( ), "Invalid HTTP Version" );

							auto major = str.pop_front( "." ).front( );
							daw::exception::daw_throw_on_true( str.empty( ), "Invalid HTTP Version" );
							auto minor = str.pop_front( );

							daw::exception::daw_throw_on_false( std::isdigit( major ) && std::isdigit( minor ),
							                                    "Invalid HTTP version" );
							result += major;
							result += '.';
							result += minor;
							std::cout << "	version: '" << result << "'\n";
							std::cout << "	result: '" << str << "'\n";
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

							str = daw::parser::trim_left( daw::make_string_view_it( bounds.cend( ), str.cend( ) ) );

							result.url = absolute_url_path_parser( str.pop_front( " " ) );

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

						boost::optional<UrlAuthInfo> url_auth_parser( daw::string_view str ) {
							if( str.empty( ) ) {
								return boost::none;
							}
							// username:password@
							auto username = str.pop_front( ":" );
							return UrlAuthInfo{url_decode( username ), url_decode( str )};
						}

						std::vector<daw::string_view> split_headers( daw::string_view str ) {
							std::vector<daw::string_view> headers;
							while( !str.empty( ) ) {
								auto header = str.pop_front( "\r\n" );
								headers.push_back( header );
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

						std::string url_host_parser( daw::string_view str ) {
							static daw::string_view const invalid_vals = R"(()<>@,;:\"/[]?={} \x09)";

							auto const first_pos = str.find_first_of( invalid_vals );
							daw::exception::daw_throw_on_false( first_pos == str.npos, "Invalid hostname" );
							return url_decode( str );
						}

						boost::optional<uint16_t> url_port_parser( daw::string_view str ) {
							if( str.empty( ) ) {
								return boost::none;
							}
							return daw::parser::parse_unsigned_int<uint16_t>( str );
						}

						http::impl::HttpUrlImpl url_parser( daw::string_view str ) {
							daw::exception::daw_throw_on_true( str.empty( ), "Unexpected empty string" );

							http::impl::HttpUrlImpl result{};
							result.scheme = str.pop_front( "://" ).to_string( );
							daw::exception::daw_throw_on_true( str.empty( ), "Missing URI scheme" );

							result.auth_info = url_auth_parser( str.try_pop_front( "@" ) );

							{
								auto pos = str.find( "/" );
								if( pos != str.npos ) {
									--pos;
								}
								daw::string_view tmp{ str.data( ), pos };
								auto port = tmp.try_pop_back( ":" );
								str.remove_prefix( pos );
								result.host = url_host_parser( tmp );
								result.port = url_port_parser( port );
							}
							result.path = absolute_url_path_parser( str );
							return result;
						}
					} // namespace impl

					daw::nodepp::lib::http::HttpAbsoluteUrlPath http_absolute_url_path_parser( daw::string_view str ) {
						return impl::absolute_url_path_parser( str );
					}

					daw::nodepp::lib::http::impl::HttpClientRequestImpl http_request_parser( daw::string_view str ) {
						daw::nodepp::lib::http::impl::HttpClientRequestImpl result;
						impl::request_parser( str, result );
						return result;
					}

					daw::nodepp::lib::http::impl::HttpUrlImpl http_url_parser( daw::string_view str ) {
						return impl::url_parser( str );
					}
				} // namespace parse
			}   // namespace http
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
