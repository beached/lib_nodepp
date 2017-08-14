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

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>

#include "base_event_emitter.h"
#include "lib_http_request.h"
#include "lib_http_server.h"
#include "lib_http_server_response.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					class HttpSiteImpl;
				} // namespace impl

				using HttpSite = std::shared_ptr<impl::HttpSiteImpl>;

				HttpSite create_http_site(
				    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

				HttpSite create_http_site( HttpServer server, daw::nodepp::base::EventEmitter emitter =
				                                                  daw::nodepp::base::create_event_emitter( ) );

				HttpSite create_http_site(
				    daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

				namespace impl {
					struct site_registration {
						std::string host; // * = any
						std::string path; // postfixing with a * means match left(will mean)
						std::function<void( daw::nodepp::lib::http::HttpClientRequest,
						                    daw::nodepp::lib::http::HttpServerResponse )>
						    listener;
						HttpClientRequestMethod method;

						site_registration( );
						~site_registration( ) = default;

						site_registration( site_registration const & ) = default;
						site_registration( site_registration &&other ) noexcept;
						site_registration &operator=( site_registration const & ) = default;
						site_registration &operator=( site_registration &&rhs ) noexcept;

						site_registration( daw::string_view Host, daw::string_view Path,
						                   daw::nodepp::lib::http::HttpClientRequestMethod Method );

						site_registration( daw::string_view Host, daw::string_view Path,
						                   daw::nodepp::lib::http::HttpClientRequestMethod Method,
						                   std::function<void( daw::nodepp::lib::http::HttpClientRequest,
						                                       daw::nodepp::lib::http::HttpServerResponse )>
						                       Listener );
					}; // site_registration

					bool operator==( site_registration const &lhs, site_registration const &rhs ) noexcept;

					class HttpSiteImpl : public daw::nodepp::base::enable_shared<HttpSiteImpl>,
					                     public daw::nodepp::base::StandardEvents<HttpSiteImpl> {
					  public:
						using registered_pages_t = std::vector<site_registration>;
						using iterator = registered_pages_t::iterator;

					  private:
						daw::nodepp::lib::http::HttpServer m_server;
						registered_pages_t m_registered_sites;
						std::unordered_map<uint16_t,
						                   std::function<void( HttpClientRequest,
						                                       daw::nodepp::lib::http::HttpServerResponse, uint16_t )>>
						    m_error_listeners;

						void sort_registered( );

						void start( );

						HttpSiteImpl( daw::nodepp::base::EventEmitter emitter );

						HttpSiteImpl( daw::nodepp::lib::http::HttpServer server,
						              daw::nodepp::base::EventEmitter emitter );

						HttpSiteImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
						              daw::nodepp::base::EventEmitter emitter );

						friend daw::nodepp::lib::http::HttpSite
						daw::nodepp::lib::http::create_http_site( daw::nodepp::base::EventEmitter emitter );

						friend daw::nodepp::lib::http::HttpSite
						daw::nodepp::lib::http::create_http_site( HttpServer server,
						                                          daw::nodepp::base::EventEmitter emitter );

						friend daw::nodepp::lib::http::HttpSite daw::nodepp::lib::http::create_http_site(
						    daw::nodepp::lib::net::SslServerConfig const &ssl_config,
						    daw::nodepp::base::EventEmitter emitter );

					  public:
						~HttpSiteImpl( ) override;

						HttpSiteImpl( HttpSiteImpl const & ) = delete;
						HttpSiteImpl &operator=( HttpSiteImpl const & ) = delete;
						HttpSiteImpl( HttpSiteImpl && ) noexcept = default;
						HttpSiteImpl &operator=( HttpSiteImpl && ) noexcept = default;

						//////////////////////////////////////////////////////////////////////////
						/// Summary:	Register a listener for a HTTP method and path on any
						///				host
						HttpSiteImpl &on_requests_for( daw::nodepp::lib::http::HttpClientRequestMethod method,
						                               std::string path,
						                               std::function<void( daw::nodepp::lib::http::HttpClientRequest,
						                                                   daw::nodepp::lib::http::HttpServerResponse )>
						                                   listener );

						//////////////////////////////////////////////////////////////////////////
						/// Summary:	Register a listener for a HTTP method and path on a
						///				specific hostname
						HttpSiteImpl &on_requests_for( daw::string_view hostname,
						                               daw::nodepp::lib::http::HttpClientRequestMethod method,
						                               std::string path,
						                               std::function<void( daw::nodepp::lib::http::HttpClientRequest,
						                                                   daw::nodepp::lib::http::HttpServerResponse )>
						                                   listener );

						void remove_site( iterator item );

						iterator end( );

						iterator match_site( daw::string_view host, daw::string_view path,
						                     daw::nodepp::lib::http::HttpClientRequestMethod method );

						bool has_error_handler( uint16_t error_no );

						//////////////////////////////////////////////////////////////////////////
						// Summary:	Use the default error handler for HTTP errors. This is the
						//			default.
						HttpSiteImpl &clear_page_error_listeners( );

						//////////////////////////////////////////////////////////////////////////
						// Summary:	Create a generic error handler
						HttpSiteImpl &on_any_page_error(
						    std::function<void( daw::nodepp::lib::http::HttpClientRequest,
						                        daw::nodepp::lib::http::HttpServerResponse, uint16_t error_no )>
						        listener );

						//////////////////////////////////////////////////////////////////////////
						// Summary:	Use the default error handler for specific HTTP error.
						HttpSiteImpl &except_on_page_error( uint16_t error_no );

						//////////////////////////////////////////////////////////////////////////
						// Summary:	Specify a callback to handle a specific page error
						HttpSiteImpl &on_page_error(
						    uint16_t error_no,
						    std::function<void( daw::nodepp::lib::http::HttpClientRequest,
						                        daw::nodepp::lib::http::HttpServerResponse, uint16_t error_number )>
						        listener );

						void emit_page_error( daw::nodepp::lib::http::HttpClientRequest request,
						                      daw::nodepp::lib::http::HttpServerResponse response, uint16_t error_no );

						void emit_listening( daw::nodepp::lib::net::EndPoint endpoint );

						void emit_request_made( HttpClientRequest request, HttpServerResponse response );

						HttpSiteImpl &on_listening( std::function<void( daw::nodepp::lib::net::EndPoint )> listener );

						HttpSiteImpl &listen_on(
						    uint16_t port,
						    daw::nodepp::lib::net::ip_version ip_ver = daw::nodepp::lib::net::ip_version::ipv4_v6,
						    uint16_t max_backlog = 511 );

					}; // class HttpSiteImpl
				}      // namespace impl

			} // namespace http

		} // namespace lib
	}     // namespace nodepp
} // namespace daw
