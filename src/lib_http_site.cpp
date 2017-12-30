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

#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>

#include <daw/daw_container_algorithm.h>
#include <daw/daw_string.h>

#include "lib_http.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					bool operator==( site_registration const &lhs, site_registration const &rhs ) noexcept {
						return ( lhs.method == rhs.method ) && ( lhs.host == rhs.host ) && ( lhs.path == rhs.path );
					}

					site_registration::site_registration( std::string Host, std::string Path, HttpClientRequestMethod Method,
					                                      std::function<void( HttpClientRequest, HttpServerResponse )> Listener )
					  : host{std::move( Host )}
					  , path{std::move( Path )}
					  , listener{std::move( Listener )}
					  , method{Method} {}

					site_registration::site_registration( std::string Host, std::string Path, HttpClientRequestMethod Method )
					  : host{std::move( Host )}
					  , path{std::move( Path )}
					  , listener{nullptr}
					  , method{Method} {}

					site_registration::site_registration( )
					  : method{HttpClientRequestMethod::Any} {}

					site_registration &site_registration::operator=( site_registration &&rhs ) noexcept {
						if( this == &rhs ) {
							return *this;
						}
						host = std::move( rhs.host );
						path = std::move( rhs.path );
						method = rhs.method;
						listener = std::move( rhs.listener );
						return *this;
					}

					site_registration::site_registration( site_registration &&other ) noexcept
					  : host{std::move( other.host )}
					  , path{std::move( other.path )}
					  , listener{std::move( other.listener )}
					  , method{other.method} {}
				} // namespace impl

				HttpSite::HttpSite( base::EventEmitter emitter )
				  : daw::nodepp::base::StandardEvents<HttpSite>{std::move( emitter )}
				  , m_server{} {}

				HttpSite::HttpSite( HttpServer server, base::EventEmitter emitter )
				  : daw::nodepp::base::StandardEvents<HttpSite>( std::move( emitter ) )
				  , m_server{std::move( server )} {}

				HttpSite::HttpSite( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				                    daw::nodepp::base::EventEmitter emitter )
				  : daw::nodepp::base::StandardEvents<HttpSite>{std::move( emitter )}
				  , m_server{ssl_config} {}

				namespace impl {
					namespace {
						std::string find_host_name( HttpClientRequest const &request ) {
							auto host_it = request.headers.find( "Host" );
							if( request.headers.end( ) == host_it || host_it->second.empty( ) ) {
								return std::string{};
							}
							return daw::string_view{host_it->second}.pop_front( ":" ).to_string( );
						}
						void handle_request_made( HttpClientRequest const &request, HttpServerResponse &response, HttpSite &self ) {
							std::string host{};
							try {
								host = find_host_name( request );
								if( host.empty( ) ) {
									return;
								}
							} catch( ... ) {
								self.emit_error( std::current_exception( ), "Error parsing host in request", "handle_request_made" );
								self.emit_page_error( request, response, 400 );
								return;
							}
							try {
								auto site = self.match_site( host, request.request_line.url.path, request.request_line.method );
								if( self.end( ) == site ) {
									self.emit_page_error( request, response, 404 );
								} else {
									site->listener( request, response );
								}
							} catch( ... ) {
								self.emit_error( std::current_exception( ), "Error parsing matching request", "handle_request_made" );
								self.emit_page_error( request, response, 400 );
								return;
							}
						}
					} // namespace
				}   // namespace impl

				void HttpSite::start( ) {
					m_server.on_error( emitter( ), "Http Server Error", "HttpSite::start" )
					  .delegate_to<daw::nodepp::lib::net::EndPoint>( "listening", emitter( ), "listening" )
					  .on_client_connected( [ obj = emitter( ), site = *this ]( HttpServerConnection connection ) mutable {
						  try {
							  connection.on_error( obj, "Connection error", "HttpSite::start#on_client_connected" )
							    .delegate_to( "client_error", obj, "error" )
							    .on_request_made( [ obj, site = std::move( site ) ]( HttpClientRequest request,
							                                                         HttpServerResponse response ) mutable {
								    try {
									    impl::handle_request_made( request, response, site );
								    } catch( ... ) {
									    obj.emit_error( std::current_exception( ), "Processing request",
									                    "HttpSite::start( )#on_request_made" );
								    }
							    } );
						  } catch( ... ) {
							  obj.emit_error( std::current_exception( ), "Error starting Http Server", "HttpSite::start" );
						  }
					  } );
				}

				void HttpSite::sort_registered( ) {
					daw::container::sort( m_registered_sites,
					                      []( impl::site_registration const &lhs, impl::site_registration const &rhs ) {
						                      return lhs.host < rhs.host;
					                      } );

					daw::container::stable_sort( m_registered_sites,
					                             []( impl::site_registration const &lhs, impl::site_registration const &rhs ) {
						                             return lhs.path < rhs.path;
					                             } );
				}

				HttpSite &HttpSite::on_requests_for( HttpClientRequestMethod method, std::string path,
				                                     std::function<void( HttpClientRequest, HttpServerResponse )> listener ) {

					m_registered_sites.emplace_back( "*", std::move( path ), method, listener );
					return *this;
				}

				HttpSite &HttpSite::on_requests_for( daw::string_view hostname, HttpClientRequestMethod method,
				                                     std::string path,
				                                     std::function<void( HttpClientRequest, HttpServerResponse )> listener ) {
					m_registered_sites.emplace_back( hostname.to_string( ), hostname, method, listener );
					return *this;
				}

				void HttpSite::remove_site( HttpSite::iterator item ) {
					m_registered_sites.erase( item );
				}

				HttpSite::iterator HttpSite::end( ) {
					return m_registered_sites.end( );
				}

				namespace {
					bool is_parent_of( boost::filesystem::path const &parent, boost::filesystem::path child ) {
						while( child.string( ).size( ) >= parent.string( ).size( ) ) {
							if( child == parent ) {
								return true;
							}
							child = child.parent_path( );
						}
						return false;
					}

					constexpr bool host_matches( daw::string_view const registered_host,
					                             daw::string_view const current_host ) noexcept {
						return ( registered_host == current_host ) || ( registered_host == "*" ) || ( current_host == "*" );
					}

					constexpr bool method_matches( HttpClientRequestMethod registered_method,
					                               HttpClientRequestMethod current_method ) noexcept {
						return ( current_method == registered_method ) || ( registered_method == HttpClientRequestMethod::Any ) ||
						       ( current_method == HttpClientRequestMethod::Any );
					}
				} // namespace

				HttpSite::iterator HttpSite::match_site( daw::string_view host, daw::string_view path,
				                                         HttpClientRequestMethod method ) {

					auto const key = impl::site_registration( host, path, method );

					return daw::container::max_element( m_registered_sites, [&key]( auto const &lhs, auto const &rhs ) {
						auto const hm = host_matches( rhs.host, key.host );
						auto const ipo = is_parent_of( rhs.path, key.path );
						auto const mm = method_matches( rhs.method, key.method );

						return hm && ipo && mm && ( lhs.path.size( ) < rhs.path.size( ) );
					} );
				}

				bool HttpSite::has_error_handler( uint16_t error_no ) {
					return std::end( m_error_listeners ) == m_error_listeners.find( error_no );
				}

				HttpSite &HttpSite::clear_page_error_listeners( ) {
					m_error_listeners.clear( );
					return *this;
				}

				HttpSite &HttpSite::on_any_page_error(
				  std::function<void( HttpClientRequest, HttpServerResponse, uint16_t error_no )> listener ) {
					m_error_listeners[0] = std::move( listener );
					return *this;
				}

				HttpSite &HttpSite::except_on_page_error( uint16_t error_no ) {
					m_error_listeners.erase( error_no );
					return *this;
				}

				HttpSite &
				HttpSite::on_page_error( uint16_t error_no,
				                         std::function<void( HttpClientRequest, HttpServerResponse, uint16_t )> listener ) {

					m_error_listeners[error_no] = std::move( listener );
					return *this;
				}

				namespace {
					void default_page_error_listener( HttpServerResponse const &response, uint16_t error_no ) {
						create_http_server_error_response( response, error_no );
					}
				} // namespace

				void HttpSite::emit_page_error( HttpClientRequest request, HttpServerResponse response, uint16_t error_no ) {
					response.reset( );
					auto err_it = m_error_listeners.find( error_no );

					std::function<void( HttpClientRequest, HttpServerResponse, uint16_t )> handler =
					  []( HttpClientRequest, HttpServerResponse resp, uint16_t err ) {
						  default_page_error_listener( resp, err );
					  };
					if( std::end( m_error_listeners ) != err_it ) {
						handler = err_it->second;
					} else if( std::end( m_error_listeners ) != ( err_it = m_error_listeners.find( 0 ) ) ) {
						handler = err_it->second;
					}
					handler( std::move( request ), response, error_no );
				}

				void HttpSite::emit_listening( daw::nodepp::lib::net::EndPoint endpoint ) {
					emitter( ).emit( "listening", std::move( endpoint ) );
				}

				HttpSite &HttpSite::listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver, uint16_t max_backlog ) {
					m_server.listen_on( port, ip_ver, max_backlog );
					return *this;
				}

				HttpSite::~HttpSite( ) = default;
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw

