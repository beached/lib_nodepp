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
#include "lib_http_static_service.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					HttpStaticServiceImpl::HttpStaticServiceImpl( std::string base_url_path,
					                                              daw::string_view local_filesystem_path,
					                                              daw::nodepp::base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<HttpStaticServiceImpl>{std::move( emitter )}
					    , m_base_path{std::move( base_url_path )}
					    , m_local_filesystem_path{boost::filesystem::canonical( local_filesystem_path.data( ))} {

						if( m_base_path.back( ) == '/' ) {
							m_base_path.pop_back( );
						}
					}

					namespace {
						bool is_parent_of( boost::filesystem::path const  & parent, boost::filesystem::path const & child ) {
							daw::string_view parent_str = parent.string( );
							daw::string_view child_str = child.string( );
							return (parent_str.size( ) < child_str.size( ))
								   && (child_str.substr( parent_str.size( ) ) == parent_str);
						}

						void process_request( HttpStaticServiceImpl & srv, daw::nodepp::lib::http::HttpClientRequest request,
						                      daw::nodepp::lib::http::HttpServerResponse response ) {


							daw::string_view requested_url = request->request_line.url.path;
							requested_url.remove_prefix( srv.get_base_path( ).size( ) );
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
							response->send_status( 200 )
									.add_header( "Content-Type", "text/plain" )
									.add_header( "Connection", "close" );

							daw::nodepp::lib::file::read_file_async( requested_file.string( ), [response](base::OptionalError error, std::shared_ptr<base::data_t> data ) {
								if( error || !data || data->empty( ) ) {
									response->end( ).close_when_writes_completed( );
									return;
								}
								response->write( *data );
							});

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

				HttpStaticService create_web_service( std::string base_url_path, std::string local_filesystem_path ) {
					return std::make_shared<impl::HttpStaticServiceImpl>( std::move( base_url_path ),
					                                                      std::move( local_filesystem_path ) );
				}

			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
