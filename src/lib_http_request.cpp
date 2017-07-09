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

#include <boost/algorithm/string/case_conv.hpp>
#include <ostream>

#include <daw/daw_utility.h>

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
					}

					throw std::runtime_error( "Unrecognized HttpRequestMethod" );
				}

				HttpClientRequestMethod http_request_method_from_string( daw::string_view Method ) {
					auto method = boost::algorithm::to_lower_copy( Method.to_string( ) );
					if( "get" == method ) {
						return HttpClientRequestMethod::Get;
					} else if( "post" == method ) {
						return HttpClientRequestMethod::Post;
					} else if( "connect" == method ) {
						return HttpClientRequestMethod::Connect;
					} else if( "delete" == method ) {
						return HttpClientRequestMethod::Delete;
					} else if( "head" == method ) {
						return HttpClientRequestMethod::Head;
					} else if( "options" == method ) {
						return HttpClientRequestMethod::Options;
					} else if( "put" == method ) {
						return HttpClientRequestMethod::Put;
					} else if( "trace" == method ) {
						return HttpClientRequestMethod::Trace;
					} else if( "any" == method ) {
						return HttpClientRequestMethod::Any;
					}
					throw std::runtime_error( "unknown http request method" );
				}

				std::ostream &operator<<( std::ostream &os, HttpClientRequestMethod method ) {
					os << to_string( method );
					return os;
				}

				std::istream &operator>>( std::istream &is, HttpClientRequestMethod &method ) {
					std::string method_string;
					is >> method_string;
					method = http_request_method_from_string( method_string );
					return is;
				}

				void HttpRequestLine::json_link_map( ) {
					link_json_streamable( "method", method );
					link_json_object( "url", url );
					link_json_string( "version", version );
				}

				void HttpClientRequestBody::json_link_map( ) {
					link_json_string( "content_type", content_type );
					link_json_string( "content", content );
				}

				HttpClientRequestHeader::HttpClientRequestHeader( std::pair<std::string, std::string> values )
				    : first{std::move( values.first )}, second{std::move( values.second )} {}

				HttpClientRequestHeader &HttpClientRequestHeader::operator=( std::pair<std::string, std::string> rhs ) {
					first = std::move( rhs.first );
					second = std::move( rhs.second );
					return *this;
				}

				void HttpClientRequestHeader::json_link_map( ) {
					link_json_string( "first", first );
					link_json_string( "second", first );
				}

				HttpClientRequestHeader::HttpClientRequestHeader( std::string First, std::string Second )
				    : first{std::move( First )}, second{std::move( Second )} {}

				void swap( HttpClientRequestHeader &lhs, HttpClientRequestHeader &rhs ) noexcept {
					using std::swap;
					swap( lhs.first, rhs.first );
					swap( lhs.second, rhs.second );
				}

				bool operator==( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first == rhs.first && lhs.second == rhs.second;
				}

				bool operator!=( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first != rhs.first && lhs.second != rhs.second;
				}

				bool operator>( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first > rhs.first && lhs.second > rhs.second;
				}

				bool operator<( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first < rhs.first && lhs.second < rhs.second;
				}

				bool operator>=( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first >= rhs.first && lhs.second >= rhs.second;
				}

				bool operator<=( HttpClientRequestHeader const &lhs, HttpClientRequestHeader const &rhs ) noexcept {
					return lhs.first <= rhs.first && lhs.second <= rhs.second;
				}

				void HttpClientRequestHeaders::json_link_map( ) {
					link_json_object_array( "headers", headers );
				}

				HttpClientRequestHeaders::HttpClientRequestHeaders( HttpClientRequestHeaders::values_type h )
				    : headers{std::move( h )} {}

				HttpClientRequestHeaders::iterator HttpClientRequestHeaders::find( daw::string_view key ) {
					auto const k = key.to_string( );
					return std::find_if( headers.begin( ), headers.end( ),
					                     [&k]( auto const &item ) { return k == item.first; } );
				}

				HttpClientRequestHeaders::const_iterator HttpClientRequestHeaders::find( daw::string_view key ) const {
					auto const k = key.to_string( );
					return std::find_if( headers.cbegin( ), headers.cend( ),
					                     [&k]( auto const &item ) { return k == item.first; } );
				}

				HttpClientRequestHeaders::reference HttpClientRequestHeaders::
				operator[]( HttpClientRequestHeaders::key_type key ) {
					return find( key )->second;
				}

				HttpClientRequestHeaders::const_reference HttpClientRequestHeaders::
				operator[]( HttpClientRequestHeaders::key_type key ) const {
					return find( key )->second;
				}

				HttpClientRequestHeaders::size_type HttpClientRequestHeaders::size( ) const noexcept {
					return headers.size( );
				}

				bool HttpClientRequestHeaders::empty( ) const noexcept {
					return headers.empty( );
				}

				namespace impl {
					void HttpClientRequestImpl::json_link_map( ) {
						link_json_object( "request", request_line );
						link_json_object( "headers", headers );
						link_json_object_optional( "body", body, boost::none );
					}
				} // namespace impl

				HttpClientRequest create_http_client_request( daw::string_view path,
				                                              HttpClientRequestMethod const &method ) {
					auto result = std::make_shared<impl::HttpClientRequestImpl>( );
					result->request_line.method = method;
					auto url = parse_url_path( path );
					if( url ) {
						result->request_line.url = *url;
					}
					return result;
				}
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
