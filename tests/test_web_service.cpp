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

#include <cstdlib>
#include <memory>

#include <daw/daw_read_file.h>
#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_link_file.h>

#include "base_task_management.h"
#include "lib_http_request.h"
#include "lib_http_site.h"
#include "lib_http_webservice.h"
#include "lib_net_server.h"

namespace {
	struct config_t {
		std::string url_path = "/";
		uint16_t port = 8080;
	};

	inline auto describe_json_class( config_t ) noexcept {
		using namespace daw::json;
		static constexpr char const n0[] = "url_path";
		static constexpr char const n1[] = "port";
		return class_description_t<json_string<n0>, json_number<n1, uint16_t>>{};
	}

	inline auto to_json_data( config_t const &value ) noexcept {
		return std::forward_as_tuple( value.url_path, value.port );
	}
} // namespace
struct X {
	int value = 0;
};

inline auto describe_json_class( X ) noexcept {
	using namespace daw::json;
	static constexpr char const n0[] = "value";
	return class_description_t<json_number<n0, int>>{};
}

inline auto to_json_data( X const &value ) noexcept {
	return std::forward_as_tuple( value.value );
}

int main( int argc, char const **argv ) {
	config_t config{};

	if( argc > 1 ) {
		try {
			auto const json_data = daw::read_file( argv[1] );
			config = daw::json::from_json<config_t>( json_data );
		} catch( std::exception const & ) {
			std::cerr << "Error parsing config file" << std::endl;
			exit( EXIT_FAILURE );
		}
	}
	std::cout << "Current config\n\n" << daw::json::to_json( config ) << '\n';

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	auto site = HttpSite{};

	site.on_listening( []( EndPoint endpoint ) {
		std::cout << "Node++ Web Service Server\n";
		std::cout << "Listening on " << endpoint << '\n';
	} );
	site.on_error( []( base::Error error ) {
		std::cerr << "Error: ";
		std::cerr << error << '\n';
	} );
	site.on_requests_for( HttpClientRequestMethod::Get, config.url_path,
	                      [&]( auto &&request, auto &&response ) {
		                      if( request.request_line.url.path != "/" ) {
			                      site.emit_page_error( request, response, 404 );
			                      return;
		                      }
		                      auto req = daw::json::to_json( request );
		                      request.from_json_string( req );

		                      response.send_status( 200 )
		                        .add_header( "Content-Type", "application/json" )
		                        .add_header( "Connection", "close" )
		                        .end( request.to_json_string( ) )
		                        .close_when_writes_completed( );
	                      } );
	site.listen_on( config.port );

	auto const ws_handler = [site]( auto &&request, auto &&response ) mutable {
		auto const query_value = request.request_line.url.query_get( "value" );
		if( !query_value ) {
			response.reset( );
			site.emit_page_error( request, response, 400 );
			return;
		}
		daw::string_view v = *query_value;
		auto resp_value = daw::json::from_json<X>( v );

		daw::exception::daw_throw_on_true( resp_value.value % 2 == 0,
		                                   "Exception in handler" );

		resp_value.value *= 2;

		response.send_status( 200 )
		  .add_header( "Content-Type", "application/json" )
		  .add_header( "Connection", "close" )
		  .end( daw::json::to_json( resp_value ) )
		  .close_when_writes_completed( );
	};

	auto test =
	  HttpWebService<>( HttpClientRequestMethod::Get, "/people", ws_handler );
	test.connect( site );

	auto teapot =
	  HttpWebService<>( HttpClientRequestMethod::Get, "/teapot",
	                    []( auto &&request, auto &&response ) {
		                    Unused( request );
		                    response.send_status( 418 )
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

	teapot.connect( site );

	base::start_service( base::StartServiceMode::OnePerCore );
	return EXIT_SUCCESS;
}
