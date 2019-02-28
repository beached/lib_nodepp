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

#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <memory>

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
	};

	inline auto describe_json_class( config_t ) noexcept {
		using namespace daw::json;
		static constexpr char const n0[] = "url_path";
		static constexpr char const n1[] = "port";
		return class_description_t<json_string<n0>, json_number<n1, uint16_t>>{};
	}

	constexpr inline auto to_json_data( config_t const &value ) noexcept {
		return std::forward_as_tuple( value.url_path, value.port );
	}
} // namespace

int main( int argc, char const **argv ) {
	config_t config;
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

	auto server = HttpServer{};

	server
	  .on_listening( []( EndPoint endpoint ) {
		  std::cout << "Node++ Web Service Server\n";
		  std::cout << "Listening on " << endpoint << '\n';
	  } )
	  .on_client_connected( []( auto server_connection ) {
		  server_connection.on_request_made( []( auto &&request, auto &response ) {
			  Unused( request );
			  // std::cout << "Request for " << req.request_line.method << " " <<
			  // req.request_line.url << '\n';
			  response.send_status( 200, "OK" )
			    .add_header( "Content-Type", "text/html" )
			    .add_header( "Connection", "close" )
			    .end(
			      R"(<html><header><title>OK</title></header><body>OK</body></html>)" )
			    .close( );
		  } );
	  } )
	  .on_error( []( auto err ) { std::cerr << err << std::endl; } )
	  .listen_on( config.port );

	base::start_service( base::StartServiceMode::Single );
	//	base::ServiceHandle::run( );
	return EXIT_SUCCESS;
}
