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

#include <daw/json/daw_json.h>
#include <daw/json/daw_json_link.h>

#include "base_work_queue.h"
#include "lib_http_request.h"
#include "lib_http_site.h"
#include "lib_http_server.h"
#include "lib_net_server.h"

struct config_t: public daw::json::JsonLink<config_t> {
	uint16_t port;
	std::string url_path;

	config_t( ):
			daw::json::JsonLink<config_t>{ },
			port{ 8080 },
			url_path{ u8"/" } {

		link_integral( "port", port );
		link_string( "url_path", url_path );
	}
	~config_t( );
};    // config_t

config_t::~config_t( ) { }

int main( int argc, char const **argv ) {
	config_t config;
	if( argc > 1 ) {
		try {
			config.decode_file( argv[1] );
		} catch( std::exception const & ) {
			std::cerr << "Error parsing config file" << std::endl;
			exit( EXIT_FAILURE );
		}
	} else {
		std::string fpath = argv[0];
		fpath += ".json";
		config.encode_file( fpath );
	}

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	auto server = create_http_server( );
	server->on_client_connected( []( HttpServerConnection server_connection ) {
		server_connection->on_request_made( []( HttpClientRequest req, HttpServerResponse resp ) {
			std::cout << "Request for " << req->request_line.method << " " << req->request_line.url << '\n';
			resp->send_status( 200, "OK").on_all_writes_completed( []( auto rsp ) {
				rsp->close( );
			} );
		});
	}).on_error( []( auto err ) {
		std::cerr << err << std::endl;
			} ).listen_on( config.port );
/*
	auto site = HttpSiteCreate( );
	site->on_listening( []( auto endpoint ) {
		std::cout << "Listening on " << endpoint << "\n";
	} ).on_requests_for( HttpClientRequestMethod::Get, config.url_path,
						 [&]( HttpClientRequest request, HttpServerResponse response ) {
							 response->on_all_writes_completed( []( auto resp ) {
										 resp->close( );
									 } ).send_status( 200 )
									 .add_header( "Content-Type", "text/html" )
									 .add_header( "Connection", "close" )
									 .end( R"(<p>Hello World!</p>)" );
						 } );
	.on_error( []( base::Error error ) {
	std::cerr << ""; //error << std::endl;
} )*/
//	site->listen_on( config.port );
	/*.on_page_error( 404, []( lib::http::HttpClientRequest request, lib::http::HttpServerResponse response, uint16_t ) {
		std::cout << "404 Request for " << request->request.url.path << " with query";
		{
			auto const & p = request->request.url.query;		
			if( p ) {
				for( auto const & item : p.get( ) ) {
					std::cout << item.serialize_to_json( ) << ",\n";
				}
			}50
		std::cout << "\n";
	} )
*/
	base::start_service( base::StartServiceMode::Single );
//	base::ServiceHandle::run( );
	return EXIT_SUCCESS;
}