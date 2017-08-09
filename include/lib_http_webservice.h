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

#include <functional>
#include <memory>
#include <set>
#include <type_traits>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_event_emitter.h"
#include "base_work_queue.h"
#include "lib_http_request.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					template<typename Handler>
					class HttpWebServiceImpl;
				}

				template<typename Handler>
				using HttpWebService = std::shared_ptr<impl::HttpWebServiceImpl<Handler>>;

				namespace impl {
					template<typename Handler>
					class HttpWebServiceImpl : public daw::nodepp::base::enable_shared<HttpWebServiceImpl<Handler>>,
					                           public daw::nodepp::base::StandardEvents<HttpWebServiceImpl<Handler>> {
						std::set<daw::nodepp::lib::http::HttpClientRequestMethod> m_method;
						std::string m_base_path;
						Handler m_handler;
						bool m_synchronous;

					  public:
						HttpWebServiceImpl( ) = delete;
						~HttpWebServiceImpl( ) = default;

						HttpWebServiceImpl(
						    std::initializer_list<daw::nodepp::lib::http::HttpClientRequestMethod> method,
						    daw::string_view base_path, Handler handler, bool synchronous = false,
						    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) )
						    : daw::nodepp::base::StandardEvents<HttpWebServiceImpl<Handler>>{std::move( emitter )}
						    , m_method{method.begin( ), method.end( )}
						    , m_base_path{base_path.to_string( )}
						    , m_handler{std::move( handler )}
						    , m_synchronous{synchronous} {

							m_method.insert( method );
							daw::exception::daw_throw_on_false( m_base_path.front( ) == '/',
							                                    "Base paths must beging with a /" );
						}

						HttpWebServiceImpl(
						    daw::nodepp::lib::http::HttpClientRequestMethod method, daw::string_view base_path,
						    Handler handler, bool synchronous = false,
						    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) )
						    : HttpWebServiceImpl{{std::move( method )},
						                         std::move( base_path ),
						                         std::move( handler ),
						                         synchronous,
						                         std::move( emitter )} {}

						HttpWebServiceImpl( HttpWebServiceImpl const & ) = default;
						HttpWebServiceImpl( HttpWebServiceImpl && ) = default;
						HttpWebServiceImpl &operator=( HttpWebServiceImpl const & ) = default;
						HttpWebServiceImpl &operator=( HttpWebServiceImpl && ) = default;

						/*						template<typename T>
						                        T from_string( daw::string_view json_text );*/

						bool is_method_allowed( daw::nodepp::lib::http::HttpClientRequestMethod method ) {
							return m_method.count( method ) != 0;
						}

						HttpWebServiceImpl &connect( HttpSite &site ) {
							auto self = this->get_weak_ptr( );
							site->delegate_to( "exit", self, "exit" ).delegate_to( "error", self, "error" );
							auto req_handler = [self]( daw::nodepp::lib::http::HttpClientRequest request,
							                           daw::nodepp::lib::http::HttpServerResponse response ) {
								HttpWebServiceImpl::run_if_valid(
								    self, "Error processing request", "HttpWebServiceImpl::connect",
								    [&]( auto self_l ) {
									    if( self_l->is_method_allowed( request->request_line.method ) ) {
										    try {
											    self_l->m_handler( request, response );
										    } catch( ... ) {
											    std::string msg =
											        "Exception in Handler while processing request for '" +
											        request->to_json_string( ) + "'";
											    self_l->emit_error( std::current_exception( ), std::move( msg ),
											                        "HttpServerImpl::handle_connection" );

											    response->send_status( 500 )
											        .add_header( "Content-Type", "text/plain" )
											        .add_header( "Connection", "close" )
											        .end( "Error processing request" )
											        .close( );
										    }
									    } else {
										    response->send_status( 405 )
										        .add_header( "Content-Type", "text/plain" )
										        .add_header( "Connection", "close" )
										        .end( "Method Not Allowed" )
										        .close( );
									    }
								    } );
							};
							for( auto const &current_method : m_method ) {
								site->on_requests_for( current_method, m_base_path, req_handler );
							}
							return *this;
						}

						//
					}; // class HttpWebService
				}      // namespace impl

				// TODO: build trait that allows for types that are jsonlink like or have a value_to_json/json_to_value
				// overload
				template<typename Handler>
				HttpWebService<Handler> create_web_service( daw::nodepp::lib::http::HttpClientRequestMethod method,
				                                            daw::string_view base_path, Handler handler,
				                                            bool synchronous = false ) {

					return std::make_shared<impl::HttpWebServiceImpl<Handler>>( method, base_path, handler,
					                                                            synchronous );
				}
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
