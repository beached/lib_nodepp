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

#include <boost/filesystem.hpp>

#include "lib_file.h"
#include "lib_file_info.h"
#include "lib_http_static_service.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					HttpStaticServiceImpl::HttpStaticServiceImpl( daw::string_view base_url_path,
					                                              daw::string_view local_filesystem_path,
					                                              daw::nodepp::base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpStaticServiceImpl>{std::move( emitter )}
					    , m_base_path{base_url_path.to_string( )}
					    , m_local_filesystem_path{boost::filesystem::canonical( local_filesystem_path.data( ) )}
					    , m_default_filenames{{"index.html"}} {

						if( m_base_path.back( ) != '/' ) {
							m_base_path += "/";
						}

						daw::exception::daw_throw_on_false( exists( m_local_filesystem_path ),
						                                    "Local filesystem web directory does not exist" );
						daw::exception::daw_throw_on_false( is_directory( m_local_filesystem_path ),
						                                    "Local filesystem web directory is not a directory" );
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

						void process_request( HttpStaticServiceImpl &srv, HttpSiteImpl &site,
						                      daw::nodepp::lib::http::HttpClientRequest const &request,
						                      daw::nodepp::lib::http::HttpServerResponse const &response ) {

							try {
								daw::string_view requested_url = request->request_line.url.path;
								requested_url.remove_prefix( srv.get_base_path( ).size( ) - 1 );
								bool path_exists = true;
								boost::filesystem::path requested_file;
								try {
									requested_file =
									    canonical( srv.get_local_filesystem_path( ) / requested_url.data( ) );
								} catch( ... ) { path_exists = false; }

								{
									bool const req_exists = exists( requested_file );
									bool const ipo = is_parent_of( srv.get_local_filesystem_path( ), requested_file );
									if( !path_exists || !req_exists || !ipo ) {
										site.emit_page_error( request, response, 404 );
										return;
									}
								}
								if( boost::filesystem::is_directory( requested_file ) ) {
									if( srv.get_default_filenames( ).empty( ) ) {
										site.emit_page_error( request, response, 404 );
										return;
									}
									bool found = false;
									for( auto const &fname : srv.get_default_filenames( ) ) {
										auto new_file = requested_file / fname;
										if( exists( new_file ) ) {
											requested_file = new_file;
											found = true;
											break;
										}
									}
									if( !found ) {
										site.emit_page_error( request, response, 404 );
										return;
									}
								}
								auto content_type =
								    daw::nodepp::lib::file::get_content_type( requested_file.string( ) );
								if( content_type.empty( ) ) {
									content_type = "application/octet-stream";
								}
								if( content_type.empty( ) ) {
									site.emit_page_error( request, response, 500 );
									return;
								}

								// Send page
								response->send_status( 200 )
								    .add_header( "Content-Type", content_type )
								    .add_header( "Connection", "close" )
								    .prepare_raw_write( boost::filesystem::file_size( requested_file ) )
								    .write_file( requested_file.string( ) )
								    .close( false );
							} catch( ... ) {
								std::string msg = "Exception in Handler while processing request for '" +
								                  request->to_json_string( ) + "'";
								srv.emit_error( std::current_exception( ), msg, "process_request" );

								site.emit_page_error( request, response, 500 );
							}
						}
					} // namespace
					HttpStaticServiceImpl::~HttpStaticServiceImpl( ) = default;

					HttpStaticServiceImpl &HttpStaticServiceImpl::connect( HttpSite const &site ) {
						emit_error_on_throw(
						    get_ptr( ), "Error while connecting", "HttpStaticServiceImpl::connect", [&]( ) {
							    auto self = this->get_weak_ptr( );
							    this->delegate_to(
							        "error", site->get_weak_ptr( ),
							        "error" ); // Site gets errors so that one 1 handler is needed for all services

							    site->delegate_to( "exit", self, "exit" )
							        .on_requests_for( daw::nodepp::lib::http::HttpClientRequestMethod::Get, m_base_path,
							                          [ self, site_w = site->get_weak_ptr( ) ](
							                              daw::nodepp::lib::http::HttpClientRequest request,
							                              daw::nodepp::lib::http::HttpServerResponse response ) {

								                          run_if_valid( self, "Error processing static request",
								                                        "HttpStaticServiceImpl::connect",
								                                        [&]( HttpStaticService self_l ) {
									                                        if( !site_w.expired( ) ) {
										                                        process_request( *self_l,
										                                                         *( site_w.lock( ) ),
										                                                         request, response );
									                                        }
								                                        } );

							                          } );
						    } );
						return *this;
					}

					std::string const &HttpStaticServiceImpl::get_base_path( ) const {
						return m_base_path;
					}

					boost::filesystem::path const &HttpStaticServiceImpl::get_local_filesystem_path( ) const {
						return m_local_filesystem_path;
					}

					std::vector<std::string> &HttpStaticServiceImpl::get_default_filenames( ) {
						return m_default_filenames;
					}

					std::vector<std::string> const &HttpStaticServiceImpl::get_default_filenames( ) const {
						return m_default_filenames;
					}
				}; // namespace impl

				HttpStaticService create_static_service( daw::string_view base_url_path,
				                                         daw::string_view local_filesystem_path ) {
					return std::make_shared<impl::HttpStaticServiceImpl>( base_url_path, local_filesystem_path );
				}
			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
