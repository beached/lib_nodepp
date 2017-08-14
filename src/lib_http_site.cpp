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

#include <daw/daw_string.h>

#include "lib_http.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					using namespace daw::nodepp;

					bool operator==( site_registration const &lhs, site_registration const &rhs ) noexcept {
						return ( lhs.method == rhs.method ) && ( lhs.host == rhs.host ) && ( lhs.path == rhs.path );
					}

					site_registration::site_registration(
					    daw::string_view Host, daw::string_view Path, HttpClientRequestMethod Method,
					    std::function<void( HttpClientRequest, HttpServerResponse )> Listener )
					    : host{Host.to_string( )}
					    , path{Path.to_string( )}
					    , listener{std::move( Listener )}
					    , method{Method} {}

					site_registration::site_registration( daw::string_view Host, daw::string_view Path,
					                                      HttpClientRequestMethod Method )
					    : host{Host.to_string( )}, path{Path.to_string( )}, listener{nullptr}, method{Method} {}

					site_registration::site_registration( ) : method{HttpClientRequestMethod::Any} {}

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

					HttpSiteImpl::HttpSiteImpl( base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpSiteImpl>{std::move( emitter )}
					    , m_server{create_http_server( )} {}

					HttpSiteImpl::HttpSiteImpl( HttpServer server, base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpSiteImpl>( std::move( emitter ) )
					    , m_server{std::move( server )} {}

					HttpSiteImpl::HttpSiteImpl( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
					                            daw::nodepp::base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpSiteImpl>{std::move( emitter )}
					    , m_server{create_http_server( ssl_config )} {}

					namespace {
						void handle_request_made( HttpClientRequest const &request, HttpServerResponse &response,
						                          HttpSite self ) {
							std::string host;
							try {
								host = [&]( ) {
									auto host_it = request->headers.find( "Host" );
									if( request->headers.end( ) == host_it || host_it->second.empty( ) ) {
										return std::string{};
									}
									auto result = daw::string::split( host_it->second, ':' );
									if( !result.empty( ) && result.size( ) <= 2 ) {
										return result[0];
									}
									return std::string{};
								}( );
							} catch( ... ) {
								self->emit_error( std::current_exception( ), "Error parsing host in request",
								                  "handle_request_made" );
								self->emit_page_error( request, response, 400 );
								return;
							}
							try {
								if( !host.empty( ) ) {
									auto site = self->match_site( host, request->request_line.url.path,
									                              request->request_line.method );
									if( self->end( ) == site ) {
										self->emit_page_error( request, response, 404 );
									} else {
										site->listener( request, response );
									}
								}
							} catch( ... ) {
								self->emit_error( std::current_exception( ), "Error parsing matching request",
								                  "handle_request_made" );
								self->emit_page_error( request, response, 400 );
								return;
							}
						}
					} // namespace

					void HttpSiteImpl::start( ) {
						auto obj = this->get_weak_ptr( );
						m_server->on_error( obj, "Http Server Error", "HttpSiteImpl::start" )
						    .delegate_to<daw::nodepp::lib::net::EndPoint>( "listening", obj, "listening" )
						    .on_client_connected( [obj]( HttpServerConnection connection ) {
							    connection->on_error( obj, "Connection error",
							                          "HttpSiteImpl::start#on_client_connected" );
							    connection->delegate_to( "client_error", obj, "error" );
							    connection->on_request_made( [obj]( HttpClientRequest request,
							                                        HttpServerResponse response ) {
								    run_if_valid( obj, "Processing request", "HttpSiteImpl::start( )#on_request_made",
								                  [&request, &response]( HttpSite self ) {
									                  handle_request_made( request, response, self );
								                  } );
							    } );
						    } );
					}

					void HttpSiteImpl::sort_registered( ) {
						daw::algorithm::sort( m_registered_sites,
						                      []( site_registration const &lhs, site_registration const &rhs ) {
							                      return lhs.host < rhs.host;
						                      } );

						daw::algorithm::stable_sort( m_registered_sites,
						                             []( site_registration const &lhs, site_registration const &rhs ) {
							                             return lhs.path < rhs.path;
						                             } );
					}

					HttpSiteImpl &HttpSiteImpl::on_requests_for(
					    HttpClientRequestMethod method, std::string path,
					    std::function<void( HttpClientRequest, HttpServerResponse )> listener ) {

						m_registered_sites.emplace_back( "*", std::move( path ), method, listener );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::on_requests_for(
					    daw::string_view hostname, HttpClientRequestMethod method, std::string path,
					    std::function<void( HttpClientRequest, HttpServerResponse )> listener ) {
						m_registered_sites.emplace_back( hostname.to_string( ), hostname, method, listener );
						return *this;
					}

					void HttpSiteImpl::remove_site( HttpSiteImpl::iterator item ) {
						m_registered_sites.erase( item );
					}

					HttpSiteImpl::iterator HttpSiteImpl::end( ) {
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
							return ( registered_host == current_host ) || ( registered_host == "*" ) ||
							       ( current_host == "*" );
						}

						constexpr bool method_matches( HttpClientRequestMethod registered_method,
						                               HttpClientRequestMethod current_method ) noexcept {
							return ( current_method == registered_method ) ||
							       ( registered_method == HttpClientRequestMethod::Any ) ||
							       ( current_method == HttpClientRequestMethod::Any );
						}
					} // namespace

					HttpSiteImpl::iterator HttpSiteImpl::match_site( daw::string_view host, daw::string_view path,
					                                                 HttpClientRequestMethod method ) {
						auto key = site_registration( host.to_string( ), path.to_string( ), method );
						auto result = m_registered_sites.end( );
						// Find longest matching site.
						for( auto it = m_registered_sites.begin( ); it != m_registered_sites.end( ); ++it ) {
							auto const hm = host_matches( it->host, key.host );
							auto const ipo = is_parent_of( it->path, key.path );
							auto const mm = method_matches( it->method, key.method );

							if( hm && ipo && mm ) {
								if( ( m_registered_sites.end( ) == result ) ||
								    ( result->path.size( ) < it->path.size( ) ) ) {
									result = it;
								}
							}
						}
						return result;
					}

					bool HttpSiteImpl::has_error_handler( uint16_t error_no ) {
						return std::end( m_error_listeners ) == m_error_listeners.find( error_no );
					}

					HttpSiteImpl &HttpSiteImpl::clear_page_error_listeners( ) {
						m_error_listeners.clear( );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::on_any_page_error(
					    std::function<void( HttpClientRequest, HttpServerResponse, uint16_t error_no )> listener ) {
						m_error_listeners[0] = std::move( listener );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::except_on_page_error( uint16_t error_no ) {
						m_error_listeners.erase( error_no );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::on_page_error(
					    uint16_t error_no,
					    std::function<void( HttpClientRequest, HttpServerResponse, uint16_t )> listener ) {

						m_error_listeners[error_no] = std::move( listener );
						return *this;
					}

					namespace {
						void default_page_error_listener( HttpServerResponse const &response, uint16_t error_no ) {
							create_http_server_error_response( response, error_no );
						}
					} // namespace

					void HttpSiteImpl::emit_page_error( HttpClientRequest request, HttpServerResponse response,
					                                    uint16_t error_no ) {
						response->reset( );
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

					void HttpSiteImpl::emit_listening( daw::nodepp::lib::net::EndPoint endpoint ) {
						emitter( )->emit( "listening", std::move( endpoint ) );
					}

					HttpSiteImpl &
					HttpSiteImpl::on_listening( std::function<void( daw::nodepp::lib::net::EndPoint )> listener ) {
						emitter( )->add_listener( "listening", std::move( listener ) );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::listen_on( uint16_t port, daw::nodepp::lib::net::ip_version ip_ver,
					                                       uint16_t max_backlog ) {
						m_server->listen_on( port, ip_ver, max_backlog );
						return *this;
					}

					HttpSiteImpl::~HttpSiteImpl( ) = default;
				} // namespace impl

				HttpSite create_http_site( daw::nodepp::base::EventEmitter emitter ) {

					HttpSite result{new impl::HttpSiteImpl{std::move( emitter )}};
					if( result ) {
						result->start( );
					}
					return result;
				}

				HttpSite create_http_site( HttpServer server, daw::nodepp::base::EventEmitter emitter ) {
					HttpSite result{new impl::HttpSiteImpl{std::move( server ), std::move( emitter )}};
					if( result ) {
						result->start( );
					}
					return result;
				}

				HttpSite create_http_site( daw::nodepp::lib::net::SslServerConfig const &ssl_config,
				                           base::EventEmitter emitter ) {

					HttpSite result{new impl::HttpSiteImpl{ssl_config, std::move( emitter )}};
					if( result ) {
						result->start( );
					}
					return result;
				}
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
