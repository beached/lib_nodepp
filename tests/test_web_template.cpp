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

#include <atomic>
#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <memory>

#include <daw/daw_parse_template.h>
#include <daw/daw_read_file.h>
#include <daw/json/daw_json_link.h>

#include "base_task_management.h"
#include "lib_http_request.h"
#include "lib_http_server.h"
#include "lib_http_site.h"
#include "lib_net_server.h"

namespace {
	struct config_t {
		std::string url_path = u8"/";
		uint16_t port = 8080;
		std::string template_file = "../test_template.shtml";
	};

	inline auto describe_json_class( config_t ) noexcept {
		using namespace daw::json;
		static constexpr char const n0[] = "url_path";
		static constexpr char const n1[] = "port";
		static constexpr char const n2[] = "template_file";
		return class_description_t<json_string<n0>, json_number<n1, uint16_t>,
		                           json_string<n2>>{};
	}

	constexpr inline auto to_json_data( config_t const &value ) noexcept {
		return std::forward_as_tuple( value.url_path, value.port, value.template_file );
	}
} // namespace

int main( int argc, char const **argv ) {
	config_t config{};

	if( argc > 1 ) {
		try {
			auto const json_data = daw::read_file( argv[1] );
			config = daw::json::from_json<config_t>( json_data );
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
		daw::filesystem::memory_mapped_file_t<char> template_file(
		  config.template_file );
		if( !template_file.is_open( ) ) {
			std::cerr << "Error opening file: " << config.template_file << std::endl;
			exit( EXIT_FAILURE );
		}
		return std::string{template_file.cbegin( ), template_file.cend( )};
	}( );

	auto p = daw::parse_template( template_str );

	std::atomic<size_t> count{0};
	p.add_callback( "counter", [&count]( ) { return count++; } );

	auto server = daw::nodepp::lib::http::HttpServer{};

	server
	  .on_listening( []( EndPoint endpoint ) {
		  std::cout << "Node++ Web Service Server\n";
		  std::cout << "Listening on " << endpoint << '\n';
	  } )
	  .on_client_connected( [&p]( auto &&server_connection ) {
		  server_connection.on_request_made(
		    [&p]( auto &&request, auto &&response ) {
			    // std::cout << "Request for " << request.request_line.method << " "
			    // << request.request_line.url << '\n';
			    if( request.request_line.url.path == "/" ) {
				    response.send_status( 200, "OK" )
				      .add_header( "Content-Type", "text/html" )
				      .add_header( "Connection", "close" )
				      .end( p.to_string( ) )
				      .close( );
			    } else {
				    response.send_status( 404, "Not Found" )
				      .add_header( "Content-Type", "text/plain" )
				      .add_header( "Connection", "close" )
				      .end( "Could not find requested page" )
				      .close( );
			    }
		    } );
	  } )
	  .on_error( []( auto err ) { std::cerr << err << std::endl; } )
	  .listen_on( config.port, daw::nodepp::lib::net::ip_version::ipv4_v6, 1024 );

	base::start_service( base::StartServiceMode::OnePerCore );
	//	base::ServiceHandle::run( );
	return EXIT_SUCCESS;
}
