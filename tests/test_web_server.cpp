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
	std::string url_path;
	uint16_t port;

	config_t( )
	  : url_path{u8"/"}
	  , port{8080} {}
	~config_t( ) = default;
	config_t( config_t const & ) = default;
	config_t( config_t && ) noexcept = default;
	config_t &operator=( config_t const & ) = default;
	config_t &operator=( config_t && ) noexcept = default;

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
			std::cerr << "Error parsing config file\n";
			exit( EXIT_FAILURE );
		}
	} else {
		boost::filesystem::path fpath{argv[0]};
		fpath /= ".json";
		// TODO config.to_file( fpath );
	}

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	HttpServer server{};

	server
	  .on_listening( []( EndPoint endpoint ) {
		  std::cout << "Node++ Web Service Server\n";
		  std::cout << "Listening on " << endpoint << '\n';
	  } )
	  .on_client_connected( []( HttpServerConnection server_connection ) {
		  server_connection->on_request_made( []( HttpClientRequest req, HttpServerResponse resp ) {
			  // std::cout << "Request for " << req->request_line.method << " " << req->request_line.url << '\n';
			  resp->send_status( 200, "OK" )
			    .add_header( "Content-Type", "text/html" )
			    .add_header( "Connection", "close" )
			    .end( R"(<html><header><title>OK</title></header><body>OK</body></html>)" )
			    .close( );
		  } );
	  } )
	  .on_error( []( auto err ) { std::cerr << err << std::endl; } )
	  .listen_on( config.port );

	base::start_service( base::StartServiceMode::Single );
	//	base::ServiceHandle::run( );
	return EXIT_SUCCESS;
}
