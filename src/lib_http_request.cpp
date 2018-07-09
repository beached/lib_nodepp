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

#include <boost/algorithm/string/case_conv.hpp>
#include <ostream>

#include <daw/daw_container_algorithm.h>
#include <daw/daw_utility.h>

#include "base_memory.h"
#include "base_types.h"
#include "lib_http_parser.h"
#include "lib_http_request.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				using namespace daw::nodepp;

				std::string to_string( HttpClientRequestMethod method ) {
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
				                          HttpClientRequestMethod const &method ) {
					os << to_string( method );
					return os;
				}

				std::istream &operator>>( std::istream &is,
				                          HttpClientRequestMethod &method ) {
					std::string method_string;
					is >> method_string;
					method =
					  http_request_method_from_string( daw::string_view{method_string} );
					return is;
				}

				void HttpRequestLine::json_link_map( ) {
					link_json_streamable( "method", method );
					link_json_string( "version", version );
					link_json_object( "url", url );
				}

				void HttpClientRequestBody::json_link_map( ) {
					link_json_string( "content_type", content_type );
					link_json_string( "content", content );
				}

				HttpClientRequestHeader::HttpClientRequestHeader(
				  std::pair<std::string, std::string> values )
				  : first{std::move( values.first )}
				  , second{std::move( values.second )} {}

				HttpClientRequestHeader &HttpClientRequestHeader::
				operator=( std::pair<std::string, std::string> rhs ) {
					first = std::move( rhs.first );
					second = std::move( rhs.second );
					return *this;
				}

				void HttpClientRequestHeader::json_link_map( ) {
					link_json_string( "first", first );
					link_json_string( "second", second );
				}

				HttpClientRequestHeader::HttpClientRequestHeader( std::string First,
				                                                  std::string Second )
				  : first{std::move( First )}
				  , second{std::move( Second )} {}

				HttpClientRequestHeader::HttpClientRequestHeader(
				  daw::string_view First, daw::string_view Second )
				  : first{First.to_string( )}
				  , second{Second.to_string( )} {}

				bool operator==( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first == rhs.first && lhs.second == rhs.second;
				}

				bool operator!=( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first != rhs.first && lhs.second != rhs.second;
				}

				bool operator>( HttpClientRequestHeader const &lhs,
				                HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first > rhs.first && lhs.second > rhs.second;
				}

				bool operator<( HttpClientRequestHeader const &lhs,
				                HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first < rhs.first && lhs.second < rhs.second;
				}

				bool operator>=( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first >= rhs.first && lhs.second >= rhs.second;
				}

				bool operator<=( HttpClientRequestHeader const &lhs,
				                 HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first <= rhs.first && lhs.second <= rhs.second;
				}

				void HttpClientRequestHeaders::json_link_map( ) {
					link_json_object_array( "headers", headers );
				}

				HttpClientRequestHeaders::HttpClientRequestHeaders(
				  HttpClientRequestHeaders::values_type h )
				  : headers{std::move( h )} {}

				HttpClientRequestHeaders::iterator
				HttpClientRequestHeaders::find( daw::string_view key ) {
					return daw::container::find_if(
					  headers, [k = key.to_string( )]( auto const &item ) {
						  return k == item.first;
					  } );
				}

				HttpClientRequestHeaders::const_iterator
				HttpClientRequestHeaders::find( daw::string_view key ) const {
					return daw::container::find_if(
					  headers, [k = key.to_string( )]( auto const &item ) {
						  return k == item.first;
					  } );
				}

				HttpClientRequestHeaders::reference HttpClientRequestHeaders::
				operator[]( HttpClientRequestHeaders::key_type key ) {
					return find( key )->second;
				}

				HttpClientRequestHeaders::const_reference HttpClientRequestHeaders::
				operator[]( HttpClientRequestHeaders::key_type key ) const {
					return find( key )->second;
				}

				HttpClientRequestHeaders::size_type
				HttpClientRequestHeaders::size( ) const noexcept {
					return headers.size( );
				}

				bool HttpClientRequestHeaders::empty( ) const noexcept {
					return headers.empty( );
				}

				void HttpClientRequest::json_link_map( ) {
					link_json_object( "request", request_line );
					link_json_object( "headers", headers );
					link_json_object_optional( "body", body, boost::none );
				}

				std::vector<base::key_value_t>
				HttpClientRequest::get_parameters( daw::string_view prefix ) const {
					// TODO: Add base_path to request object
					std::vector<base::key_value_t> result;

					daw::string_view path = request_line.url.path;

					daw::exception::daw_throw_on_false(
					  prefix == path.substr( 0, prefix.length( ) ),
					  "Prefix does not match beggining of URL path" );

					path =
					  path.substr( prefix.length( ) + ( prefix.back( ) == '/' ? 0 : 1 ) );

					while( !path.empty( ) ) {
						base::key_value_t current_item{};
						{
							auto tmp = path.pop_front( "/" );
							if( path.empty( ) ) {
								result.emplace_back( path.to_string( ), "" );
								break;
							}
							current_item.key = tmp;
						}
						current_item.value = path.pop_front( "/" );
						result.push_back( std::move( current_item ) );
					}
					// *************************
					/*
					auto pos = path.find_first_of( '/' );
					while( path && pos != path.npos ) {
					  base::key_value_t current_item;
					  current_item.key = path.substr( 0, pos );
					  if( pos < path.size( ) ) {
					    path.remove_prefix( pos + 1 );
					  } else {
					    result.push_back( std::move( current_item ) );
					    break;
					  }
					  if( path.empty( ) ) {
					    break;
					  }
					  pos = path.find_first_of( '/' );
					  current_item.value = path.substr( 0, pos );
					  result.push_back( std::move( current_item ) );
					  if( pos < path.size( ) ) {
					    path.remove_prefix( pos + 1 );
					  } else {
					    break;
					  }
					  pos = path.find_first_of( '/' );
					}
					*/
					if( path ) {
						result.emplace_back( path.to_string( ), "" );
					}
					return result;
				}

				HttpClientRequest
				create_http_client_request( daw::string_view path,
				                            HttpClientRequestMethod const &method ) {
					HttpClientRequest result{};
					result.request_line.method = method;
					auto url = parse_url_path( path );
					if( url ) {
						result.request_line.url = *url;
					}
					return result;
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
