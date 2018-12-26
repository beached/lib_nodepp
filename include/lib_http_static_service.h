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
#include <memory>
#include <string>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_event_emitter.h"
#include "lib_file_info.h"
#include "lib_http_request.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				template<typename EventEmitter>
				class basic_http_static_service_t;

				namespace impl {
					bool is_parent_of( boost::filesystem::path const &parent,
					                   boost::filesystem::path child );

					template<typename EventEmitter, typename Req, typename Resp>
					void process_request( basic_http_static_service_t<EventEmitter> &srv,
					                      basic_http_site_t<EventEmitter> &site,
					                      Req &&request, Resp &&response ) {
						try {
							daw::string_view requested_url = request.request_line.url.path;
							requested_url.remove_prefix( srv.get_base_path( ).size( ) - 1 );
							bool path_exists = true;
							boost::filesystem::path requested_file;
							try {
								requested_file = canonical( srv.get_local_filesystem_path( ) /
								                            requested_url.data( ) );
							} catch( ... ) { path_exists = false; }

							{
								bool const req_exists = exists( requested_file );
								bool const ipo = is_parent_of( srv.get_local_filesystem_path( ),
								                               requested_file );
								if( !path_exists or !req_exists or !ipo ) {
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
							  lib::file::get_content_type( requested_file.string( ) );
							if( content_type.empty( ) ) {
								content_type = "application/octet-stream";
							}
							if( content_type.empty( ) ) {
								site.emit_page_error( request, response, 500 );
								return;
							}

							// Send page
							response.send_status( 200 )
							  .add_header( "Content-Type", content_type )
							  .add_header( "Connection", "close" )
							  .prepare_raw_write(
							    boost::filesystem::file_size( requested_file ) )
							  .write_file( requested_file.string( ) )
							  .close( false );
						} catch( ... ) {
							std::string msg =
							  "Exception in Handler while processing request for '" +
							  request.to_json_string( ) + "'";
							srv.emit_error( std::current_exception( ), msg,
							                "process_request" );

							site.emit_page_error( request, response, 500 );
						}
					}
				} // namespace impl

				template<typename EventEmitter>
				class basic_http_static_service_t
				  : public base::BasicStandardEvents<
				      basic_http_static_service_t<EventEmitter>, EventEmitter> {

					using base::BasicStandardEvents<
					  basic_http_static_service_t<EventEmitter>,
					  EventEmitter>::emit_error;
					using base::BasicStandardEvents<
					  basic_http_static_service_t<EventEmitter>,
					  EventEmitter>::delgate_to;

					std::string m_base_path;
					boost::filesystem::path m_local_filesystem_path;
					std::vector<std::string> m_default_filenames;

				public:
					using base::BasicStandardEvents<
					  basic_http_static_service_t<EventEmitter>, EventEmitter>::emitter;

					explicit basic_http_static_service_t(
					  daw::string_view base_url_path,
					  daw::string_view local_filesystem_path,
					  EventEmitter &&emitter = EventEmitter( ) )
					  : base::BasicStandardEvents<
					      basic_http_static_service_t<EventEmitter>, EventEmitter>(
					      daw::move( emitter ) )
					  , m_base_path( base_url_path.to_string( ) )
					  , m_local_filesystem_path( boost::filesystem::canonical(
					      local_filesystem_path.to_string( ).c_str( ) ) )
					  , m_default_filenames( {"index.html"} ) {

						if( m_base_path.back( ) != '/' ) {
							m_base_path += "/";
						}

						daw::exception::precondition_check(
						  exists( m_local_filesystem_path ),
						  "Local filesystem web directory does not exist" );
						daw::exception::precondition_check(
						  is_directory( m_local_filesystem_path ),
						  "Local filesystem web directory is not a directory" );
					}

					basic_http_static_service_t &
					connect( basic_http_site_t<EventEmitter> &site ) {
						try {
							delegate_to( "error", site.emitter( ), "error" );
							site.delegate_to( "exit", emitter( ), "exit" );

							site.on_requests_for(
							  HttpClientRequestMethod::Get, m_base_path,
							  [serv = mutable_capture( *this ),
							   site = mutable_capture( site )]( auto &&request,
							                                    auto &&response ) {
								  impl::process_request(
								    *serv, *site, std::forward<decltype( request )>( request ),
								    std::forward<decltype( response )>( response ) );
							  } );
						} catch( ... ) {
							emit_error( std::current_exception( ), "Error while connecting",
							            "basic_http_static_service_t::connect" );
						}
						return *this;
					}

					std::string const &get_base_path( ) const {
						return m_base_path;
					}

					boost::filesystem::path const &get_local_filesystem_path( ) const {
						return m_local_filesystem_path;
					}

					std::vector<std::string> &get_default_filenames( ) {
						return m_default_filenames;
					}

					std::vector<std::string> const &get_default_filenames( ) const {
						return m_default_filenames;
					}
				};
				using HttpStaticService =
				  basic_http_static_service_t<base::StandardEventEmitter>;
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
