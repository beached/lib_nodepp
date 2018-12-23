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

#include <boost/filesystem/path.hpp>
#include <cstdint>
#include <string>
#include <type_traits>
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
				template<typename EventEmitter>
				class basic_http_site_t;

				namespace hs_impl {
					bool is_parent_of( boost::filesystem::path const &parent,
					                   boost::filesystem::path child );
					std::string find_host_name( HttpClientRequest const &request );

					template<typename EventEmitter>
					void default_page_error_listener(
					  HttpServerResponse<EventEmitter> const &response,
					  uint16_t error_no ) {
						create_http_server_error_response( response, error_no );
					}

					template<typename EventEmitter>
					void handle_request_made( HttpClientRequest const &request,
					                          HttpServerResponse<EventEmitter> &response,
					                          basic_http_site_t<EventEmitter> &self ) {

						auto host = std::string( );
						try {
							host = find_host_name( request );
							if( host.empty( ) ) {
								return;
							}
						} catch( ... ) {
							self.emit_error( std::current_exception( ),
							                 "Error parsing host in request",
							                 "handle_request_made" );
							self.emit_page_error( request, response, 400 );
							return;
						}
						try {
							auto site = self.match_site( host, request.request_line.url.path,
							                             request.request_line.method );
							if( self.end( ) == site ) {
								self.emit_page_error( request, response, 404 );
							} else {
								site->listener( request, response );
							}
						} catch( ... ) {
							self.emit_error( std::current_exception( ),
							                 "Error parsing matching request",
							                 "handle_request_made" );
							self.emit_page_error( request, response, 400 );
							return;
						}
					}

					template<typename EventEmitter = base::StandardEventEmitter>
					struct site_registration {
						std::string host{}; // * = any
						std::string
						  path{}; // postfixing with a * means match left(will mean)
						std::function<void( HttpClientRequest,
						                    HttpServerResponse<EventEmitter> )>
						  listener{};
						HttpClientRequestMethod method = HttpClientRequestMethod::Any;

						site_registration( std::string Host, std::string Path,
						                   HttpClientRequestMethod Method )
						  : host( daw::move( Host ) )
						  , path( daw::move( Path ) )
						  , method( Method ) {}

						site_registration( ) = default;

						template<typename Listener>
						site_registration( std::string Host, std::string Path,
						                   HttpClientRequestMethod Method, Listener &&l )
						  : host( daw::move( Host ) )
						  , path( daw::move( Path ) )
						  , listener( std::forward<Listener>( l ) )
						  , method( Method ) {

							static_assert(
							  std::is_invocable_v<std::decay_t<Listener>, HttpClientRequest,
							                      HttpServerResponse<EventEmitter>>,
							  "Listener must take arguments of type "
							  "HttpClientRequest and HttpServerResponse" );
						}

					}; // site_registration

					template<typename EventEmitter>
					bool
					operator==( site_registration<EventEmitter> const &lhs,
					            site_registration<EventEmitter> const &rhs ) noexcept {
						return ( lhs.method == rhs.method ) and ( lhs.host == rhs.host ) and
						       ( lhs.path == rhs.path );
					}
				} // namespace hs_impl

				template<typename EventEmitter>
				class basic_http_site_t
				  : public base::BasicStandardEvents<basic_http_site_t<EventEmitter>,
				                                     EventEmitter> {
				public:
					using registered_pages_t =
					  std::vector<hs_impl::site_registration<EventEmitter>>;
					using iterator = typename registered_pages_t::iterator;

				private:
					basic_http_server_t<EventEmitter> m_server{};
					registered_pages_t m_registered_sites{};
					std::unordered_map<
					  uint16_t,
					  std::function<void( HttpClientRequest,
					                      HttpServerResponse<EventEmitter>, uint16_t )>>
					  m_error_listeners{};

					void sort_registered( ) {
						daw::container::sort(
						  m_registered_sites,
						  []( hs_impl::site_registration<EventEmitter> const &lhs,
						      hs_impl::site_registration<EventEmitter> const &rhs ) {
							  return lhs.host < rhs.host;
						  } );

						daw::container::stable_sort(
						  m_registered_sites,
						  []( hs_impl::site_registration<EventEmitter> const &lhs,
						      hs_impl::site_registration<EventEmitter> const &rhs ) {
							  return lhs.path < rhs.path;
						  } );
					}

					void start( ) {
						m_server
						  .on_error( this->emitter( ), "Http Server Error",
						             "basic_http_site_t::start" )
						  .template delegate_to<net::EndPoint>(
						    "listening", this->emitter( ), "listening" )
						  .on_client_connected(
						    [obj = mutable_capture( this->emitter( ) ),
						     site = mutable_capture( *this )](
						      basic_http_server_connection_t<EventEmitter> connection ) {
							    try {
								    connection
								      .on_error(
								        *obj, "Connection error",
								        "basic_http_site_t::start#on_client_connected" )
								      .delegate_to( "client_error", *obj, "error" )
								      .on_request_made(
								        [obj, site = daw::move( site )](
								          HttpClientRequest request,
								          HttpServerResponse<EventEmitter> response ) {
									        try {
										        hs_impl::handle_request_made( daw::move( request ),
										                                      daw::move( response ),
										                                      *site );
									        } catch( ... ) {
										        obj->emit_error(
										          std::current_exception( ), "Processing request",
										          "basic_http_site_t::start( )#on_request_made" );
									        }
								        } );
							    } catch( ... ) {
								    obj->emit_error( std::current_exception( ),
								                     "Error starting Http Server",
								                     "basic_http_site_t::start" );
							    }
						    } );
					}

				public:
					basic_http_site_t( ) = default;

					explicit basic_http_site_t( EventEmitter &&emitter )
					  : base::BasicStandardEvents<basic_http_site_t<EventEmitter>,
					                              EventEmitter>( daw::move( emitter ) ) {}

					explicit basic_http_site_t( basic_http_server_t<EventEmitter> server )
					  : m_server( daw::move( server ) ) {}

					basic_http_site_t( basic_http_server_t<EventEmitter> server,
					                            EventEmitter &&emitter )
					  : base::BasicStandardEvents<basic_http_site_t<EventEmitter>,
					                              EventEmitter>( daw::move( emitter ) )
					  , m_server( daw::move( server ) ) {}

					explicit basic_http_site_t( net::SslServerConfig const &ssl_config )
					  : m_server( ssl_config ) {}

					basic_http_site_t( net::SslServerConfig const &ssl_config,
					                            EventEmitter &&emitter )
					  : base::BasicStandardEvents<basic_http_site_t<EventEmitter>,
					                              EventEmitter>( daw::move( emitter ) )
					  , m_server( ssl_config ) {}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Register a listener for a HTTP method and path on any
					///				host
					template<typename Listener>
					basic_http_site_t &on_requests_for( HttpClientRequestMethod method,
					                                    std::string path,
					                                    Listener &&listener ) {
						static_assert(
						  std::is_invocable_v<std::decay_t<Listener>, HttpClientRequest,
						                      HttpServerResponse<EventEmitter>>,
						  "Listener must accept HttpClientRequest and "
						  "HttpServerResponse as arguments" );

						m_registered_sites.emplace_back(
						  "*", daw::move( path ), method,
						  std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Register a listener for a HTTP method and path on a
					///				specific hostname
					template<typename Listener>
					basic_http_site_t &on_requests_for( daw::string_view hostname,
					                                    HttpClientRequestMethod method,
					                                    std::string path,
					                                    Listener &&listener ) {
						Unused( path );
						static_assert(
						  std::is_invocable_v<std::decay_t<Listener>, HttpClientRequest,
						                      HttpServerResponse<EventEmitter>>,
						  "Listener must accept HttpClientRequest and "
						  "HttpServerResponse as arguments" );

						m_registered_sites.emplace_back(
						  hostname.to_string( ), hostname, method,
						  std::forward<Listener>( listener ) );
						return *this;
					}

					void remove_site( iterator item ) {
						m_registered_sites.erase( item );
					}

					iterator end( ) {
						return m_registered_sites.end( );
					}

					iterator match_site( daw::string_view host, daw::string_view path,
					                     HttpClientRequestMethod method ) {

						auto const key =
						  hs_impl::site_registration<EventEmitter>( host, path, method );

						return daw::container::max_element(
						  m_registered_sites, [&key]( auto const &lhs, auto const &rhs ) {
							  auto const hm = host_matches( rhs.host, key.host );
							  auto const ipo = is_parent_of( rhs.path, key.path );
							  auto const mm = method_matches( rhs.method, key.method );

							  return hm and ipo and mm and
							         ( lhs.path.size( ) < rhs.path.size( ) );
						  } );
					}

					bool has_error_handler( uint16_t error_no ) {
						return std::end( m_error_listeners ) ==
						       m_error_listeners.find( error_no );
					}

					//////////////////////////////////////////////////////////////////////////
					// @brief	Use the default error handler for HTTP errors. This is the
					//			default.
					basic_http_site_t &clear_page_error_listeners( ) {
						m_error_listeners.clear( );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					// @brief	Create a generic error handler
					template<typename Listener>
					basic_http_site_t &on_any_page_error( Listener &&listener ) {
						static_assert(
						  std::is_invocable_v<std::decay_t<Listener>, HttpClientRequest,
						                      HttpServerResponse, uint16_t /*error_no*/>,
						  "Listener must accept HttpClientRequest, HttpServerResponse, and "
						  "uint16_t as arguments" );

						m_error_listeners[0] = std::forward<Listener>( listener );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					// @brief	Use the default error handler for specific HTTP error.
					basic_http_site_t &except_on_page_error( uint16_t error_no ) {
						m_error_listeners.erase( error_no );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					// @brief	Specify a callback to handle a specific page error
					template<typename Listener>
					basic_http_site_t &on_page_error( uint16_t error_no,
					                                  Listener &&listener ) {
						//	static_assert(
						//	  std::is_invocable_v<std::decay_t<Listener>, HttpClientRequest,
						//	                     HttpServerResponse, uint16_t /*err*/>,
						//	  "Listener does not accept arguments HttpClientRequest, "
						//	  "HttpServerResponse, and uint16_t" );

						m_error_listeners[error_no] = std::forward<Listener>( listener );
						return *this;
					}

					void emit_page_error( HttpClientRequest request,
					                      HttpServerResponse<EventEmitter> response,
					                      uint16_t error_no ) {
						response.reset( );
						auto err_it = m_error_listeners.find( error_no );

						if( std::end( m_error_listeners ) != err_it ) {
							( err_it->second )( daw::move( request ), std::move( response ),
							                    error_no );
							return;
						} else if( std::end( m_error_listeners ) !=
						           ( err_it = m_error_listeners.find( 0 ) ) ) {
							( err_it->second )( daw::move( request ), std::move( response ),
							                    error_no );
							return;
						}
						hs_impl::default_page_error_listener( daw::move( response ),
						                                      error_no );
					}

					void emit_listening( net::EndPoint endpoint ) {
						this->emitter( ).emit( "listening", daw::move( endpoint ) );
					}

					void emit_request_made( HttpClientRequest request,
					                        HttpServerResponse<EventEmitter> response ) {
						this->emitter( ).emit( "request_made", daw::move( request ),
						                       daw::move( response ) );
					}

					template<typename Listener>
					basic_http_site_t &on_listening( Listener &&listener ) {
						base::add_listener<net::EndPoint>(
						  "listening", this->emitter( ),
						  std::forward<Listener>( listener ) );
						return *this;
					}

					basic_http_site_t &
					listen_on( uint16_t port,
					           net::ip_version ip_ver = net::ip_version::ipv4_v6,
					           uint16_t max_backlog = 511 ) {

						m_server.listen_on( port, ip_ver, max_backlog );
						return *this;
					}
				}; // class basic_http_site_t

				using HttpSite = basic_http_site_t<base::StandardEventEmitter>;
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
