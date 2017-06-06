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

#include <boost/algorithm/string/case_conv.hpp>
#include <ostream>

#include "base_types.h"
#include "lib_http_parser.h"
#include "lib_http_request.h"
#include <daw/daw_utility.h>
#include <daw/json/daw_json.h>

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				using namespace daw::nodepp;
				using namespace daw::nodepp::base::json;

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

				std::string value_to_json( std::string const &name, HttpClientRequestMethod method ) {
					return daw::json::generate::value_to_json( name, to_string( method ) );
				}

				HttpClientRequestMethod http_request_method_from_string( boost::string_view Method ) {
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

				HttpRequestLine::HttpRequestLine( )
				    : daw::json::JsonLink<HttpRequestLine>{}, method{}, url{}, version{} {

					set_links( );
				}

				HttpRequestLine::HttpRequestLine( HttpRequestLine const &other )
				    : daw::json::JsonLink<HttpRequestLine>{}
				    , method{other.method}
				    , url{other.url}
				    , version{other.version} {

					set_links( );
				}

				HttpRequestLine::HttpRequestLine( HttpRequestLine &&other )
				    : daw::json::JsonLink<HttpRequestLine>{}
				    , method{std::move( other.method )}
				    , url{std::move( other.url )}
				    , version{std::move( other.version )} {

					set_links( );
				}

				HttpRequestLine &HttpRequestLine::operator=( HttpRequestLine const &rhs ) {
					if( this != &rhs ) {
						using std::swap;
						HttpRequestLine tmp{rhs};
						swap( *this, tmp );
					}
					return *this;
				}

				HttpRequestLine &HttpRequestLine::operator=( HttpRequestLine &&rhs ) {
					if( this != &rhs ) {
						using std::swap;
						HttpRequestLine tmp{std::move( rhs )};
						swap( *this, tmp );
					}
					return *this;
				}

				HttpRequestLine::~HttpRequestLine( ) {}

				void HttpRequestLine::set_links( ) {
					this->link_streamable( "method", method );
					this->link_object( "url", url );
					this->link_string( "version", version );
				}

				HttpClientRequestBody::HttpClientRequestBody( )
				    : daw::json::JsonLink<HttpClientRequestBody>{}, content_type{}, content{} {

					set_links( );
				}

				HttpClientRequestBody::HttpClientRequestBody( HttpClientRequestBody const &other )
				    : daw::json::JsonLink<HttpClientRequestBody>{}
				    , content_type{other.content_type}
				    , content{other.content} {

					set_links( );
				}

				HttpClientRequestBody::HttpClientRequestBody( HttpClientRequestBody &&other )
				    : daw::json::JsonLink<HttpClientRequestBody>{}
				    , content_type{std::move( other.content_type )}
				    , content{std::move( other.content )} {

					set_links( );
				}

				HttpClientRequestBody &HttpClientRequestBody::operator=( HttpClientRequestBody const &rhs ) {
					if( this != &rhs ) {
						using std::swap;
						HttpClientRequestBody tmp{rhs};
						swap( *this, tmp );
					}
					return *this;
				}

				HttpClientRequestBody &HttpClientRequestBody::operator=( HttpClientRequestBody &&rhs ) {
					if( this != &rhs ) {
						using std::swap;
						HttpClientRequestBody tmp{std::move( rhs )};
						swap( *this, tmp );
					}
					return *this;
				}

				HttpClientRequestBody::~HttpClientRequestBody( ) {}

				void HttpClientRequestBody::set_links( ) {
					this->link_string( "content_type", content_type );
					this->link_string( "content", content );
				}

				void HttpClientRequestHeaders::set_links( ) {
					link_map( "headers", headers );
				}

				HttpClientRequestHeaders::HttpClientRequestHeaders( HttpClientRequestHeaders::container_type h )
				    : daw::json::JsonLink<HttpClientRequestHeaders>{}
				    , daw::mixins::MapLikeProxy<HttpClientRequestHeaders,
				                                std::unordered_map<std::string, std::string>>{}
				    , headers{std::move( h )} {

					set_links( );
				}

				HttpClientRequestHeaders::HttpClientRequestHeaders( HttpClientRequestHeaders const &other )
				    : daw::json::JsonLink<HttpClientRequestHeaders>{}
				    , daw::mixins::MapLikeProxy<HttpClientRequestHeaders,
				                                std::unordered_map<std::string, std::string>>{other}
				    , headers{other.headers} {

					set_links( );
				}

				HttpClientRequestHeaders::HttpClientRequestHeaders( HttpClientRequestHeaders &&other )
				    : daw::json::JsonLink<HttpClientRequestHeaders>{}
				    , daw::mixins::MapLikeProxy<HttpClientRequestHeaders,
				                                std::unordered_map<std::string, std::string>>{std::move( other )}
				    , headers{std::move( other.headers )} {

					set_links( );
				}

				HttpClientRequestHeaders &HttpClientRequestHeaders::operator=( HttpClientRequestHeaders const &rhs ) {
					if( this != &rhs ) {
						using std::swap;
						HttpClientRequestHeaders tmp{rhs};
						swap( *this, tmp );
					}
					return *this;
				}

				HttpClientRequestHeaders &HttpClientRequestHeaders::operator=( HttpClientRequestHeaders &&rhs ) {
					if( this != &rhs ) {
						using std::swap;
						HttpClientRequestHeaders tmp{std::move( rhs )};
						swap( *this, tmp );
					}
					return *this;
				}

				HttpClientRequestHeaders::~HttpClientRequestHeaders( ) {}

				HttpClientRequestHeaders::iterator HttpClientRequestHeaders::find( boost::string_view key ) {
					return headers.find( key.to_string( ) );
				}

				HttpClientRequestHeaders::const_iterator
				HttpClientRequestHeaders::find( boost::string_view key ) const {
					return headers.find( key.to_string( ) );
				}

				namespace impl {
					HttpClientRequestImpl::HttpClientRequestImpl( )
					    : daw::json::JsonLink<HttpClientRequestImpl>{}, request_line{}, headers{}, body{} {

						set_links( );
					}

					HttpClientRequestImpl::HttpClientRequestImpl( HttpClientRequestImpl const &other )
					    : daw::json::JsonLink<HttpClientRequestImpl>{}
					    , request_line{other.request_line}
					    , headers{other.headers}
					    , body{other.body} {

						set_links( );
					}

					HttpClientRequestImpl::HttpClientRequestImpl( HttpClientRequestImpl &&other )
					    : daw::json::JsonLink<HttpClientRequestImpl>{}
					    , request_line{std::move( other.request_line )}
					    , headers{std::move( other.headers )}
					    , body{std::move( other.body )} {

						set_links( );
					}

					HttpClientRequestImpl &HttpClientRequestImpl::operator=( HttpClientRequestImpl const &rhs ) {
						if( this != &rhs ) {
							using std::swap;
							HttpClientRequestImpl tmp{rhs};
							swap( *this, tmp );
						}
						return *this;
					}

					HttpClientRequestImpl &HttpClientRequestImpl::operator=( HttpClientRequestImpl &&rhs ) {
						if( this != &rhs ) {
							using std::swap;
							HttpClientRequestImpl tmp{std::move( rhs )};
							swap( *this, tmp );
						}
						return *this;
					}

					HttpClientRequestImpl::~HttpClientRequestImpl( ) {}

					void HttpClientRequestImpl::set_links( ) {
						this->link_object( "request", request_line );
						this->link_map( "headers", headers );
						this->link_object( "body", body );
					}
				} // namespace impl

				HttpClientRequest create_http_client_request( boost::string_view path,
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
