// The MIT License (MIT)
//
// Copyright (c) 2014-2016 Darrell Wright
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

#include <cstdlib>
#include <memory>

#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_link_file.h>

#include "base_task_management.h"
#include "lib_http_request.h"
#include "lib_http_server.h"
#include "lib_http_site.h"
#include "lib_net_server.h"

struct config_t : public daw::json::daw_json_link<config_t> {
	uint16_t port;
	std::string url_path;

	config_t( ) : port{8080}, url_path{u8"/"} {}
	~config_t( ) = default;
	config_t( config_t const & ) = default;
	config_t( config_t && ) = default;
	config_t &operator=( config_t const & ) = default;
	config_t &operator=( config_t && ) = default;

	static void json_link_map( ) {
		link_json_integer( "port", port );
		link_json_string( "url_path", url_path );
	}
}; // config_t

int main( int argc, char const **argv ) {
	config_t config;
	if( argc > 1 ) {
		try {
			config = daw::json::from_file<config_t>( argv[1] );
		} catch( std::exception const & ) {
			std::cerr << "Error parsing config file" << std::endl;
			exit( EXIT_FAILURE );
		}
	} else {
		std::string fpath = argv[0];
		fpath += ".json";
		// TODO config.to_file( fpath );
	}

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;
	using base::Error;

	auto site = create_http_site( );

	site->on_listening( []( auto endpoint ) { std::cout << "Listening on " << endpoint << '\n'; } )
	    .on_requests_for( HttpClientRequestMethod::Get, config.url_path,
	                      [&]( HttpClientRequest request, HttpServerResponse response ) {
		                      response->send_status( 200 )
		                          .add_header( "Content-Type", "text/html" )
		                          .add_header( "Connection", "close" )
		                          .end( R"(<p>Hello World!</p>)" )
		                          .close( );
	                      } )
	    .on_requests_for( HttpClientRequestMethod::Get, "/status",
	                      [&]( HttpClientRequest request, HttpServerResponse response ) {
		                      response->send_status( 200 )
		                          .add_header( "Content-Type", "text/html" )
		                          .add_header( "Connection", "close" )
		                          .end( R"(<p>OK</p>)" )
		                          .close( );
	                      } )
	    .on_error( []( Error error ) { std::cerr << error << '\n'; } )
	    .on_page_error( 404,
	                    []( HttpClientRequest request, HttpServerResponse response, uint16_t /*error_no*/ ) {
		                    std::cout << "404 Request for " << request->request_line.url.path << " with query";
		                    auto const &q = request->request_line.url.query;
		                    for( auto const &item : q ) {
			                    std::cout << item.to_json_string( ) << ",\n";
		                    }
		                    std::cout << '\n';
		                    response->send_status( 404 )
		                        .add_header( "Content-Type", "text/plain" )
		                        .add_header( "Connection", "close" )
		                        .end( R"(Nothing to see here )" )
		                        .close( );
	                    } )
	    .listen_on( config.port );

	base::start_service( base::StartServiceMode::Single );
	return EXIT_SUCCESS;
}
