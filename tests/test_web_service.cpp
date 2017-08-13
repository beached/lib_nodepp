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
#include "lib_http_site.h"
#include "lib_http_webservice.h"
#include "lib_net_server.h"

struct config_t : public daw::json::daw_json_link<config_t> {
	uint16_t port;
	std::string url_path;

	config_t( ) : port{8080}, url_path{"/"} {}

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
	}
	std::cout << "Current config\n\n" << config.to_json_string( ) << '\n';

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	struct X : public daw::json::daw_json_link<X> {
		int value;

		explicit X( int val = 0 ) : value{val} {}

		static void json_link_map( ) {
			link_json_integer( "value", value );
		}
	};

	auto site = create_http_site( );
	site->on_listening( []( EndPoint endpoint ) {
		    std::cout << "Node++ Web Service Server\n";
		    std::cout << "Listening on " << endpoint << '\n';
	    } )
	    .on_error( []( base::Error error ) {
		    std::cerr << "Error: ";
		    std::cerr << error << '\n';
	    } )
	    .on_requests_for( HttpClientRequestMethod::Get, config.url_path,
	                      [&]( HttpClientRequest request, HttpServerResponse response ) {
		                      if( request->request_line.url.path != "/" ) {
			                      site->emit_page_error( request, response, 404 );
			                      return;
		                      }
		                      auto req = request->to_json_string( );
		                      request->from_json_string( req );

		                      response->send_status( 200 )
		                          .add_header( "Content-Type", "application/json" )
		                          .add_header( "Connection", "close" )
		                          .end( request->to_json_string( ) )
		                          .close_when_writes_completed( );
	                      } )
	    .listen_on( config.port );

	auto const ws_handler = [site]( HttpClientRequest request, HttpServerResponse response ) {
		auto const query_value = request->request_line.url.query_get( "value" );
		if( !query_value ) {
			response.reset( );
			site->emit_page_error( request, response, 400 );
			return;
		}
		daw::string_view v = *query_value;
		auto resp_value = daw::json::daw_json_link<X>::from_json_string( v ).result;

		daw::exception::daw_throw_on_true( resp_value.value % 2 == 0, "Exception in handler" );

		resp_value.value *= 2;

		response->send_status( 200 )
		    .add_header( "Content-Type", "application/json" )
		    .add_header( "Connection", "close" )
		    .end( resp_value.to_json_string( ) )
		    .close_when_writes_completed( );
	};

	auto test = create_web_service( HttpClientRequestMethod::Get, "/people", ws_handler );
	test->connect( site );

	auto teapot = create_web_service( HttpClientRequestMethod::Get, "/teapot", []( auto request, auto response ) {
		response->send_status( 418 )
		    .add_header( "Content-Type", "text/plain" )
		    .add_header( "Connection", "close" )
		    .end(
R"(I'm a little teapot short and stout.
Here is my handle.
Here is my spout.
When I get all steamed up,
Hear me shout!
Just tip me over
And pour me out

I'm a clever teapot, yes it's true.
Here's an example of what I can do.
I can turn my handle to a spout.
Just tip me over and pour me out)" )
		    .close_when_writes_completed( );
	} );
	teapot->connect( site );

	base::start_service( base::StartServiceMode::Single );
	return EXIT_SUCCESS;
}
