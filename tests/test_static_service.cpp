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

#include "base_work_queue.h"
#include "lib_http_request.h"
#include "lib_http_site.h"
#include "lib_http_static_service.h"

struct config_t : public daw::json::daw_json_link<config_t> {
	uint16_t port;
	std::string url_path;
	std::string file_system_path;
	std::vector<std::string> default_files;
	std::string mime_db;

	config_t( ):
		port{ 8080 },
		url_path{ "/" },
		file_system_path{ "./web_files" },
		default_files{ },
		mime_db{ } { }

	static void json_link_map( ) {
		link_json_integer( "port", port );
		link_json_string( "url_path", url_path );
		link_json_string( "file_system_path", file_system_path );
		link_json_string_array( "default_files", default_files );
		link_json_string( "mime_db", mime_db );
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
	} 
	std::cout << "Current config\n\n" << config.to_json_string( ) << '\n';

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	auto site = create_http_site( );
	site->on_listening( []( EndPoint endpoint ) {
		    std::cout << "Node++ Static HTTP Server\n";
		    std::cout << "Listening on " << endpoint << '\n';
	    } )
	    .on_error( []( base::Error error ) {
			std::cerr << "Error: ";
			std::cerr << error << '\n';
	    } )
	    .listen_on( config.port );

	auto service = create_static_service( config.url_path, config.file_system_path );
	service->connect( site );

	base::start_service( base::StartServiceMode::OnePerCore );
	return EXIT_SUCCESS;
}
