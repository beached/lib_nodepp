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

#include <boost/filesystem/path.hpp>
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
					    , m_local_filesystem_path{boost::filesystem::canonical( local_filesystem_path.data( ))} {

						if( m_base_path.back( ) != '/' ) {
							m_base_path += "/";
						}
					}

					namespace {
						bool is_parent_of( boost::filesystem::path const & parent, boost::filesystem::path child ) {
							child = child.parent_path( );
							while( child.string( ).size( ) >= parent.string( ).size( ) ) {
								if( child == parent ) {
									return true;
								}
								child = child.parent_path( );
							}
							return false;
						}

						void process_request( HttpStaticServiceImpl & srv, daw::nodepp::lib::http::HttpClientRequest request,
						                      daw::nodepp::lib::http::HttpServerResponse response ) {


							daw::string_view requested_url = request->request_line.url.path;
							requested_url.remove_prefix( srv.get_base_path( ).size( ) - 1 );
							auto const requested_file =
							    canonical( srv.get_local_filesystem_path( ) / requested_url.data( ) );

							if( !exists( requested_file ) || !is_parent_of( srv.get_local_filesystem_path( ), requested_file ) ) {
								response->send_status( 404 )
										.add_header( "Content-Type", "text/plain" )
										.add_header( "Connection", "close" )
										.end( "Not found" )
										.close_when_writes_completed( );
								return;
							}
							auto const content_type = daw::nodepp::lib::file::get_content_type( requested_file.string( ) );
							std::ifstream ifs{ requested_file.string( ).c_str( ), std::ifstream::in | std::ifstream::binary };
							if( content_type.empty( ) || !ifs ) {
								response->send_status( 500 )
										.add_header( "Content-Type", "text/plain" )
										.add_header( "Connection", "close" )
										.end( "Unexpected file type" )
										.close_when_writes_completed( );
								return;
							}
							base::data_t buffer;
							buffer.resize( 2048 );

							response->send_status( 200 )
									.add_header( "Content-Type", content_type )
									.add_header( "Connection", "close" );

							auto read_count = ifs.readsome( buffer.data( ), 2048 );
							while( ifs.good( ) && read_count > 0 ) {
								buffer.resize( read_count );
								response->write( buffer );
								read_count = ifs.readsome( buffer.data( ), 2048 );
							}
							response->end( ).close_when_writes_completed( );
						}
					} // namespace anonymous

					HttpStaticServiceImpl &HttpStaticServiceImpl::connect( HttpSite &site ) {
							auto self = this->get_weak_ptr( );
						    site->delegate_to( "exit", self, "exit" )
						        .delegate_to( "error", self, "error" )
						        .on_requests_for( daw::nodepp::lib::http::HttpClientRequestMethod::Get, m_base_path,
						                          [self]( daw::nodepp::lib::http::HttpClientRequest request,
						                                  daw::nodepp::lib::http::HttpServerResponse response ) {

							                          run_if_valid( self, "Error processing static request",
							                                        "HttpStaticServiceImpl::connect", [&]( HttpStaticService self_l ) {
								                                        process_request( *self_l, request, response );
							                                        } );

						                          } );
						    return *this;
					}

					std::string const & HttpStaticServiceImpl::get_base_path( ) const {
						return m_base_path;
					}

					boost::filesystem::path const & HttpStaticServiceImpl::get_local_filesystem_path( ) const {
						return m_local_filesystem_path;
					}

					HttpStaticServiceImpl::~HttpStaticServiceImpl( ) {}
				}; // namespace impl

				HttpStaticService create_static_service( daw::string_view base_url_path, daw::string_view local_filesystem_path ) {
					return std::make_shared<impl::HttpStaticServiceImpl>( base_url_path, local_filesystem_path );
				}

			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
