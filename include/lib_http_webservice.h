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

#pragma once

#include <functional>
#include <memory>
#include <set>
#include <type_traits>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_event_emitter.h"
#include "lib_http_request.h"
#include "lib_http_server_response.h"
#include "lib_http_site.h"
#include "lib_http_webservice.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				template<typename EventEmitter = base::StandardEventEmitter>
				class HttpWebService
				  : public base::BasicStandardEvents<HttpWebService<EventEmitter>,
				                                     EventEmitter> {

					using handler_t = std::function<void(
					  HttpClientRequest, HttpServerResponse<EventEmitter> )>;

					std::set<HttpClientRequestMethod> m_method;
					std::string m_base_path;
					handler_t m_handler;
					bool m_synchronous;

					template<typename Handler>
					handler_t make_handler( Handler &&handler ) {
						static_assert(
						  std::is_invocable_v<std::decay_t<Handler>, HttpClientRequest,
						                      HttpServerResponse<EventEmitter>>,
						  "Handler must take a HttpClientRequest and a "
						  "HttpServerResponse as arguments" );

						handler_t result =
						  [handler = mutable_capture( std::forward<Handler>( handler ) )](
						    auto &&req, auto &&resp ) {
							  ( *handler )( std::forward<decltype( req )>( req ),
							                std::forward<decltype( resp )>( resp ) );
						  };
						return result;
					}

				public:
					template<typename Handler>
					HttpWebService( std::initializer_list<HttpClientRequestMethod> method,
					                daw::string_view base_path, Handler &&handler,
					                bool synchronous = false,
					                base::StandardEventEmitter &&emitter =
					                  base::StandardEventEmitter{} )
					  : base::StandardEvents<HttpWebService>( daw::move( emitter ) )
					  , m_method( method.begin( ), method.end( ) )
					  , m_base_path( base_path.to_string( ) )
					  , m_handler( make_handler( std::forward<Handler>( handler ) ) )
					  , m_synchronous( synchronous ) {

						m_method.insert( method );
						daw::exception::precondition_check(
						  m_base_path.front( ) == '/', "Base paths must beging with a /" );
					}

					template<typename Handler>
					HttpWebService( HttpClientRequestMethod method,
					                daw::string_view base_path, Handler &&handler,
					                bool synchronous = false,
					                base::StandardEventEmitter &&emitter =
					                  base::StandardEventEmitter{} )
					  : HttpWebService( {method}, base_path,
					                    std::forward<Handler>( handler ), synchronous,
					                    daw::move( emitter ) ) {}

					bool is_method_allowed( http::HttpClientRequestMethod method ) {
						return m_method.count( method ) != 0;
					}

					HttpWebService &connect( basic_http_site_t<EventEmitter> &site ) {
						site.delegate_to( "exit", this->emitter( ), "exit" );
						site.delegate_to( "error", this->emitter( ), "error" );

						auto req_handler = [self = mutable_capture( *this )](
						                     auto &&request, auto &&response ) {
							try {
								if( self->is_method_allowed( request.request_line.method ) ) {
									try {
										self->m_handler( request, response );
									} catch( ... ) {
										std::string msg =
										  "Exception in Handler while processing request for '" +
										  request.to_json_string( ) + "'";
										self->emit_error( std::current_exception( ),
										                  daw::move( msg ),
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
								self->emit_error( std::current_exception( ),
								                  "Error processing request",
								                  "HttpWebService::connect" );
							}
						};
						for( auto current_method : m_method ) {
							site.on_requests_for( current_method, m_base_path, req_handler );
						}
						return *this;
					}
				}; // class HttpWebService
			}    // namespace http
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
