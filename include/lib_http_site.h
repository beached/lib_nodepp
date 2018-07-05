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
					struct site_registration {
						std::string host; // * = any
						std::string path; // postfixing with a * means match left(will mean)
						std::function<void( HttpClientRequest, HttpServerResponse )>
						  listener;
						HttpClientRequestMethod method;

						site_registration( );

						/*
						~site_registration( ) = default;

						site_registration( site_registration const & ) = default;

						site_registration( site_registration &&other ) noexcept;

						site_registration &operator=( site_registration const & ) = default;

						site_registration &operator=( site_registration &&rhs ) noexcept;
						*/
						site_registration(
						  std::string Host, std::string Path,
						  daw::nodepp::lib::http::HttpClientRequestMethod Method );

						template<typename Listener>
						site_registration( std::string Host, std::string Path,
						                   HttpClientRequestMethod Method, Listener &&l )
						  : host( std::move( Host ) )
						  , path( std::move( Path ) )
						  , listener( std::forward<Listener>( l ) )
						  , method( Method ) {

							static_assert( daw::is_callable_v<Listener, HttpClientRequest,
							                                  HttpServerResponse>,
							               "Listener must take arguments of type "
							               "HttpClientRequest and HttpServerResponse" );
						}

					}; // site_registration

					bool operator==( site_registration const &lhs,
					                 site_registration const &rhs ) noexcept;

				} // namespace impl

				class HttpSite : public daw::nodepp::base::StandardEvents<HttpSite> {
				public:
					using registered_pages_t = std::vector<impl::site_registration>;
					using iterator = registered_pages_t::iterator;

				private:
					daw::nodepp::lib::http::HttpServer m_server;
					registered_pages_t m_registered_sites;
					std::unordered_map<
					  uint16_t, std::function<void( HttpClientRequest, HttpServerResponse,
					                                uint16_t )>>
					  m_error_listeners;

					void sort_registered( );

					void start( );

				public:
					explicit HttpSite( daw::nodepp::base::EventEmitter &&emitter =
					                     daw::nodepp::base::EventEmitter( ) );

					explicit HttpSite( daw::nodepp::lib::http::HttpServer server,
					                   daw::nodepp::base::EventEmitter &&emitter =
					                     daw::nodepp::base::EventEmitter( ) );

					explicit HttpSite(
					  daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					  daw::nodepp::base::EventEmitter &&emitter =
					    daw::nodepp::base::EventEmitter( ) );

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Register a listener for a HTTP method and path on any
					///				host
					template<typename Listener>
					HttpSite &on_requests_for( HttpClientRequestMethod method,
					                           std::string path, Listener &&listener ) {
						static_assert( daw::is_callable_v<Listener, HttpClientRequest,
						                                  HttpServerResponse>,
						               "Listener must accept HttpClientRequest and "
						               "HttpServerResponse as arguments" );

						m_registered_sites.emplace_back(
						  "*", std::move( path ), method,
						  std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Register a listener for a HTTP method and path on a
					///				specific hostname
					template<typename Listener>
					HttpSite &on_requests_for( daw::string_view hostname,
					                           HttpClientRequestMethod method,
					                           std::string path, Listener &&listener ) {
						static_assert( daw::is_callable_v<Listener, HttpClientRequest,
						                                  HttpServerResponse>,
						               "Listener must accept HttpClientRequest and "
						               "HttpServerResponse as arguments" );

						m_registered_sites.emplace_back(
						  hostname.to_string( ), hostname, method,
						  std::forward<Listener>( listener ) );
						return *this;
					}

					void remove_site( iterator item );

					iterator end( );

					iterator
					match_site( daw::string_view host, daw::string_view path,
					            daw::nodepp::lib::http::HttpClientRequestMethod method );

					bool has_error_handler( uint16_t error_no );

					//////////////////////////////////////////////////////////////////////////
					// @brief	Use the default error handler for HTTP errors. This is the
					//			default.
					HttpSite &clear_page_error_listeners( );

					//////////////////////////////////////////////////////////////////////////
					// @brief	Create a generic error handler
					template<typename Listener>
					HttpSite &on_any_page_error( Listener &&listener ) {
						static_assert(
						  daw::is_callable_v<Listener, HttpClientRequest,
						                     HttpServerResponse, uint16_t /*error_no*/>,
						  "Listener must accept HttpClientRequest, HttpServerResponse, and "
						  "uint16_t as arguments" );

						m_error_listeners[0] = std::forward<Listener>( listener );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					// @brief	Use the default error handler for specific HTTP error.
					HttpSite &except_on_page_error( uint16_t error_no );

					//////////////////////////////////////////////////////////////////////////
					// @brief	Specify a callback to handle a specific page error
					template<typename Listener>
					HttpSite &on_page_error( uint16_t error_no, Listener &&listener ) {
						static_assert(
						  daw::is_callable_v<Listener, HttpClientRequest,
						                     HttpServerResponse, uint16_t /*err*/>,
						  "Listener does not accept arguments HttpClientRequest, "
						  "HttpServerResponse, and uint16_t" );

						m_error_listeners[error_no] = std::forward<Listener>( listener );
						return *this;
					}

					void
					emit_page_error( daw::nodepp::lib::http::HttpClientRequest request,
					                 daw::nodepp::lib::http::HttpServerResponse response,
					                 uint16_t error_no );

					void emit_listening( daw::nodepp::lib::net::EndPoint endpoint );

					void emit_request_made( HttpClientRequest request,
					                        HttpServerResponse response );

					template<typename Listener>
					HttpSite &on_listening( Listener &&listener ) {
						emitter( ).template add_listener<daw::nodepp::lib::net::EndPoint>(
						  "listening", std::forward<Listener>( listener ) );
						return *this;
					}

					HttpSite &listen_on( uint16_t port,
					                     daw::nodepp::lib::net::ip_version ip_ver =
					                       daw::nodepp::lib::net::ip_version::ipv4_v6,
					                     uint16_t max_backlog = 511 );

				}; // class HttpSite
			}    // namespace http
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
