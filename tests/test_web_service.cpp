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
#include "lib_http_webservice.h"
#include "lib_net_server.h"

struct config_t : public daw::json::daw_json_link<config_t> {
	uint16_t port;
	std::string url_path;

	config_t( ) = default;
	~config_t( ) = default;
	config_t( config_t const & ) = default;
	config_t( config_t && ) = default;
	config_t & operator=( config_t const & ) = default;
	config_t & operator=( config_t && ) = default;


	static void json_link_map( ) {
		link_json_integer( "port", port );
		link_json_string( "url_path", url_path );
	}

}; // config_t

template<typename Container, typename T>
void if_exists_do( Container &container, T const &key, std::function<void( typename Container::iterator it )> action ) {
	auto it = std::find( std::begin( container ), std::end( container ), key );
	if( std::end( container ) != it ) {
		action( it );
	}
}

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
		config.port = 8080;
		config.url_path = "/";
		std::string fpath = argv[0];
		fpath += ".json";
		// TODO config.to_file( fpath );
	}

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	struct X : public daw::json::daw_json_link<X> {
		int value;

		X( int val = 0 ) : value{std::move( val )} {}

		static void json_link_map( ) {
			link_json_integer( "value", value );
		}
	};

	auto ws_handler = []( HttpClientRequest request, HttpServerResponse response ) {
		auto query_value = request->request_line.url.query_get( "value" );
		if( query_value ) {
			daw::string_view v = *query_value;
			auto resp_value = daw::json::daw_json_link<X>::from_json_string( v ).result;

			resp_value.value *= 2;
			response->send_status( 200 )
			    .add_header( "Content-Type", "application/json" )
			    .add_header( "Connection", "close" )
			    .end( resp_value.to_json_string( ) )
			    .close_when_writes_completed( );
		} else {
			response->send_status( 400 )
			    .add_header( "Content-Type", "application/json" )
			    .add_header( "Connection", "close" )
			    .end( "Invalid query" )
			    .close_when_writes_completed( );
		}
	};

	auto test = create_web_service( HttpClientRequestMethod::Get, "/people", ws_handler );

	auto site = create_http_site( );

	test->connect( site );

	site->on_listening(
	        []( daw::nodepp::lib::net::EndPoint endpoint ) { std::cout << "Listening on " << endpoint << "\n"; } )
	    .on_requests_for( HttpClientRequestMethod::Get, config.url_path,
	                      [&]( HttpClientRequest request, HttpServerResponse response ) {
		                      auto req = request->to_json_string( );
		                      request->from_json_string( req );

		                      response->send_status( 200 )
		                          .add_header( "Content-Type", "application/json" )
		                          .add_header( "Connection", "close" )
		                          .end( request->to_json_string( ) )
		                          .close_when_writes_completed( );
	                      } )
	    .on_error( []( base::Error error ) { std::cerr << error << std::endl; } )
	    .on_page_error( 404,
	                    []( lib::http::HttpClientRequest request, lib::http::HttpServerResponse response,
	                        uint16_t /*error_number*/ ) { 
				
							response->send_status( 404 )
							.add_header( "Content-Type", "text/plain" )
							.add_header( "Connection", "close" )
							.end( "Johnny Five is alive\r\n" )
							.close_when_writes_completed( );
	                    } )

	    .listen_on( config.port );

	base::start_service( base::StartServiceMode::Single );
	return EXIT_SUCCESS;
}
