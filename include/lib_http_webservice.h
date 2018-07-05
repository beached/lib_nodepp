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
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				class HttpWebService
				  : public daw::nodepp::base::StandardEvents<HttpWebService> {
					std::set<daw::nodepp::lib::http::HttpClientRequestMethod> m_method;
					std::string m_base_path;
					using handler_t =
					  std::function<void( HttpClientRequest, HttpServerResponse )>;
					handler_t m_handler;
					bool m_synchronous;

					template<typename Handler>
					handler_t make_handler( Handler &&handler ) {
						static_assert( daw::is_callable_v<Handler, HttpClientRequest,
						                                  HttpServerResponse>,
						               "Handler must take a HttpClientRequest and a "
						               "HttpServerResponse as arguments" );

						handler_t result = [handler = std::forward<Handler>( handler )](
						                     HttpClientRequest req,
						                     HttpServerResponse resp ) mutable {
							handler( std::move( req ), std::move( resp ) );
						};
						return result;
					}

				public:
					HttpWebService( ) = delete;
					~HttpWebService( );

					template<typename Handler>
					HttpWebService( std::initializer_list<HttpClientRequestMethod> method,
					                daw::string_view base_path, Handler &&handler,
					                bool synchronous = false,
					                base::EventEmitter &&emitter = base::EventEmitter{} )
					  : base::StandardEvents<HttpWebService>( std::move( emitter ) )
					  , m_method( method.begin( ), method.end( ) )
					  , m_base_path( base_path.to_string( ) )
					  , m_handler( make_handler( std::forward<Handler>( handler ) ) )
					  , m_synchronous( synchronous ) {

						m_method.insert( method );
						daw::exception::daw_throw_on_false(
						  m_base_path.front( ) == '/', "Base paths must beging with a /" );
					}

					template<typename Handler>
					HttpWebService( HttpClientRequestMethod method,
					                daw::string_view base_path, Handler handler,
					                bool synchronous = false,
					                base::EventEmitter &&emitter = base::EventEmitter{} )
					  : HttpWebService{{method},
					                   base_path,
					                   std::move( handler ),
					                   synchronous,
					                   std::move( emitter )} {}

					HttpWebService( HttpWebService const & ) = default;
					HttpWebService( HttpWebService && ) noexcept = default;
					HttpWebService &operator=( HttpWebService const & ) = default;
					HttpWebService &operator=( HttpWebService && ) noexcept = default;

					bool is_method_allowed(
					  daw::nodepp::lib::http::HttpClientRequestMethod method );

					HttpWebService &connect( HttpSite &site );
				}; // class HttpWebService
			}    // namespace http
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
