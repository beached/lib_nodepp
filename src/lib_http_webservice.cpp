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

#include <memory>
#include <set>
#include <type_traits>

#include <daw/json/daw_json_link.h>

#include "lib_http_request.h"
#include "lib_http_site.h"
#include "lib_http_webservice.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				bool HttpWebService::is_method_allowed(
				  daw::nodepp::lib::http::HttpClientRequestMethod method ) {
					return m_method.count( method ) != 0;
				}

				HttpWebService &HttpWebService::connect( HttpSite &site ) {
					site.delegate_to( "exit", emitter( ), "exit" );
					site.delegate_to( "error", emitter( ), "error" );

					auto req_handler = [self =
					                      *this]( HttpClientRequest request,
					                              HttpServerResponse response ) mutable {
						try {
							if( self.is_method_allowed( request.request_line.method ) ) {
								try {
									self.m_handler( request, response );
								} catch( ... ) {
									std::string msg =
									  "Exception in Handler while processing request for '" +
									  request.to_json_string( ) + "'";
									self.emit_error( std::current_exception( ), std::move( msg ),
									                 "HttpServer::handle_connection" );

									response.send_status( 500 )
									  .add_header( "Content-Type", "text/plain" )
									  .add_header( "Connection", "close" )
									  .end( "Error processing request" )
									  .close( );
								}
							} else {
								response.send_status( 405 )
								  .add_header( "Content-Type", "text/plain" )
								  .add_header( "Connection", "close" )
								  .end( "Method Not Allowed" )
								  .close( );
							}
						} catch( ... ) {
							self.emit_error( std::current_exception( ),
							                 "Error processing request",
							                 "HttpWebService::connect" );
						}
					};
					for( auto const &current_method : m_method ) {
						site.on_requests_for( current_method, m_base_path, req_handler );
					}
					return *this;
				}

				HttpWebService::~HttpWebService( ) = default;
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
