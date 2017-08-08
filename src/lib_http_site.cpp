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
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <daw/daw_string.h>
#include <daw/daw_string_view.h>

#include "lib_http.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					using namespace daw::nodepp;

					bool site_registration::operator==( site_registration const &rhs ) const {
						return rhs.host == host && rhs.path == path && rhs.method == method;
					}

					site_registration::site_registration(
					    daw::string_view Host, daw::string_view Path, HttpClientRequestMethod Method,
					    std::function<void( HttpClientRequest, HttpServerResponse )> Listener )
					    : host( Host.to_string( ) )
					    , path( Path.to_string( ) )
					    , method( std::move( Method ) )
					    , listener( std::move( Listener ) ) {}

					site_registration::site_registration( daw::string_view Host, daw::string_view Path,
					                                      HttpClientRequestMethod Method )
					    : host{Host.to_string( )}
					    , path{Path.to_string( )}
					    , method{std::move( Method )}
					    , listener{nullptr} {}

					HttpSiteImpl::HttpSiteImpl( base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpSiteImpl>{emitter}
					    , m_server{create_http_server( )}
					    , m_registered_sites{}
					    , m_error_listeners{} {}

					HttpSiteImpl::HttpSiteImpl( HttpServer server, base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpSiteImpl>( std::move( emitter ) )
					    , m_server( std::move( server ) )
					    , m_registered_sites( )
					    , m_error_listeners( ) {}

					HttpSiteImpl::HttpSiteImpl( daw::nodepp::lib::net::SSLConfig const &ssl_config,
					                            daw::nodepp::base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpSiteImpl>{emitter}
					    , m_server{create_http_server( ssl_config )}
					    , m_registered_sites{}
					    , m_error_listeners{} {}

					HttpSiteImpl::~HttpSiteImpl( ) {}

					void HttpSiteImpl::start( ) {
						auto obj = this->get_weak_ptr( );
						m_server->on_error( obj, "Child" )
						    .delegate_to<daw::nodepp::lib::net::EndPoint>( "listening", obj, "listening" )
						    .on_client_connected( [obj]( HttpServerConnection connection ) {
							    connection->on_error( obj, "child connection" )
							        .on_request_made( [obj]( HttpClientRequest request, HttpServerResponse response ) {
								        run_if_valid(
								            obj, "Processing request", "HttpSiteImpl::start( )#on_request_made",
								            [&request, &response]( HttpSite self ) {
									            auto host = [&]( ) {
										            auto host_it = request->headers.find( "Host" );
										            if( request->headers.end( ) == host_it || "" == host_it->second ) {
											            return std::string{};
										            }
										            auto result = daw::string::split( host_it->second, ':' );
										            if( 0 < result.size( ) && result.size( ) <= 2 ) {
											            return result[0];
										            }
										            return std::string{};
									            }( );
									            if( "" == host ) {
										            self->emit_page_error( request, response, 400 );
									            } else {
										            auto site = self->match_site( host, request->request_line.url.path,
										                                          request->request_line.method );
										            if( self->end( ) == site ) {
											            self->emit_page_error( request, response, 404 );
										            } else {
											            site->listener( request, response );
										            }
									            }
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

						m_registered_sites.emplace_back( "*", std::move( path ), std::move( method ), listener );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::on_requests_for(
					    daw::string_view hostname, HttpClientRequestMethod method, std::string path,
					    std::function<void( HttpClientRequest, HttpServerResponse )> listener ) {
						m_registered_sites.emplace_back( hostname.to_string( ), std::move( hostname ),
						                                 std::move( method ), listener );
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
						m_error_listeners[0] = listener;
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

					void HttpSiteImpl::default_page_error_listener( HttpClientRequest,
					                                                HttpServerResponse response, uint16_t error_no ) {
						auto msg = HttpStatusCodes( error_no );
						if( msg.first != error_no ) {
							msg.first = error_no;
							msg.second = "Error";
						}
						response->send_status( msg.first, msg.second )
						    .add_header( "Content-Type", "text/plain" )
						    .add_header( "Connection", "close" )
						    .end( std::to_string( msg.first ) + " " + msg.second + "\r\n" )
						    .close( );
					}

					void HttpSiteImpl::emit_page_error( HttpClientRequest request, HttpServerResponse response,
					                                    uint16_t error_no ) {
						response->reset( );
						auto err_it = m_error_listeners.find( error_no );
						std::function<void( HttpClientRequest, HttpServerResponse, uint16_t )> handler =
						    std::bind( &HttpSiteImpl::default_page_error_listener, this, std::placeholders::_1,
						               std::placeholders::_2, std::placeholders::_3 );

						if( std::end( m_error_listeners ) != err_it ) {
							handler = err_it->second;
						} else if( std::end( m_error_listeners ) != ( err_it = m_error_listeners.find( 0 ) ) ) {
							handler = err_it->second;
						}
						handler( request, response, error_no );
					}

					void HttpSiteImpl::emit_listening( daw::nodepp::lib::net::EndPoint endpoint ) {
						emitter( )->emit( "listening", std::move( endpoint ) );
					}

					HttpSiteImpl &
					HttpSiteImpl::on_listening( std::function<void( daw::nodepp::lib::net::EndPoint )> listener ) {
						emitter( )->add_listener( "listening", listener );
						return *this;
					}

					HttpSiteImpl &HttpSiteImpl::listen_on( uint16_t port ) {
						m_server->listen_on( port );
						return *this;
					}
					HttpSite HttpSiteImpl::create( base::EventEmitter emitter ) {
						HttpSite result{new HttpSiteImpl( std::move( emitter ) )};
						if( result ) {
							result->start( );
						}
						return result;
					}

					HttpSite HttpSiteImpl::create( net::SSLConfig const & ssl_config, base::EventEmitter emitter ) {
						HttpSite result{new HttpSiteImpl( ssl_config, std::move( emitter ) )};
						if( result ) {
							result->start( );
						}
						return result;
					}

					HttpSite HttpSiteImpl::create( HttpServer server, base::EventEmitter emitter ) {
						auto result = HttpSite( new HttpSiteImpl( std::move( server ), std::move( emitter ) ) );
						if( result ) {
							result->start( );
						}
						return result;
					}
				} // namespace impl

				HttpSite create_http_site( daw::nodepp::base::EventEmitter emitter ) {
					return impl::HttpSiteImpl::create( std::move( emitter ) );
				}

				HttpSite create_http_site( HttpServer server, daw::nodepp::base::EventEmitter emitter ) {
					return impl::HttpSiteImpl::create( std::move( server ), std::move( emitter ) );
				}

				HttpSite create_http_site( daw::nodepp::lib::net::SSLConfig const & ssl_config, base::EventEmitter emitter ) {
					return impl::HttpSiteImpl::create( ssl_config, std::move( emitter ) );
				}
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
