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

#include <atomic>
#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <memory>

#include <daw/daw_parse_template.h>
#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_link_file.h>

#include "base_task_management.h"
#include "lib_http_request.h"
#include "lib_http_server.h"
#include "lib_http_site.h"
#include "lib_net_server.h"

namespace {
	struct config_t : public daw::json::daw_json_link<config_t> {
		std::string url_path;
		uint16_t port;
		std::string template_file;

		config_t( )
		  : url_path{u8"/"}
		  , port{8080}
		  , template_file{"../test_template.shtml"} {}

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
} // namespace

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

	auto const template_str = [&]( ) {
		daw::filesystem::memory_mapped_file_t<char> template_file( config.template_file );
		if( !template_file.is_open( ) ) {
			std::cerr << "Error opening file: " << config.template_file << std::endl;
			exit( EXIT_FAILURE );
		}
		return std::string{template_file.cbegin( ), template_file.cend( )};
	}( );

	daw::parse_template p{template_str};

	std::atomic<size_t> count{0};
	p.add_callback( "counter", [&count]( ) { return count++; } );

	daw::nodepp::lib::http::HttpServer server{};
	server
	  .on_listening( []( EndPoint endpoint ) {
		  std::cout << "Node++ Web Service Server\n";
		  std::cout << "Listening on " << endpoint << '\n';
	  } )
	  .on_client_connected( [&p]( HttpServerConnection server_connection ) {
		  server_connection.on_request_made( [&p]( HttpClientRequest req, HttpServerResponse resp ) {
			  // std::cout << "Request for " << req->request_line.method << " " << req->request_line.url << '\n';
			  if( req->request_line.url.path == "/" ) {
				  resp->send_status( 200, "OK" )
				    .add_header( "Content-Type", "text/html" )
				    .add_header( "Connection", "close" )
				    .end( p.to_string( ) )
				    .close( );
			  } else {
				  resp->send_status( 404, "Not Found" )
				    .add_header( "Content-Type", "text/plain" )
				    .add_header( "Connection", "close" )
				    .end( "Could not find requested page" )
				    .close( );
			  }
		  } );
	  } )
	  .on_error( []( auto err ) { std::cerr << err << std::endl; } )
	  .listen_on( config.port, daw::nodepp::lib::net::ip_version::ipv6, 1024 );

	base::start_service( base::StartServiceMode::OnePerCore );
	//	base::ServiceHandle::run( );
	return EXIT_SUCCESS;
}
