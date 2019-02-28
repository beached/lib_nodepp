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
#include <optional>

#include <daw/daw_read_file.h>
#include <daw/json/daw_json_link.h>

#include "base_task_management.h"
#include "lib_http_request.h"
#include "lib_http_site.h"
#include "lib_http_static_service.h"

namespace {
	struct config_t {
		std::string url_path = "/";
		std::string file_system_path = "./web_files";
		std::vector<std::string> default_files{};
		std::string mime_db{};
		std::optional<daw::nodepp::lib::net::SslServerConfig> ssl_config{};
		uint16_t port = 8080U;

		config_t( ) = default;
	};

	inline auto describe_json_class( config_t ) noexcept {
		using namespace daw::json;
		static constexpr char const n0[] = "url_path";
		static constexpr char const n1[] = "file_system_path";
		static constexpr char const n2[] = "default_files";
		static constexpr char const n3[] = "mime_db";
		static constexpr char const n4[] = "ssl_config";
		static constexpr char const n5[] = "port";

		return class_description_t<
		  json_string<n0>, json_string<n1>,
		  json_array<n2, std::vector<std::string>, json_string<no_name>>,
		  json_string<n3>,
		  json_nullable<json_class<n4, daw::nodepp::lib::net::SslServerConfig>>,
		  json_number<n5, uint16_t>>{};
	}

	constexpr inline auto to_json_data( config_t const &value ) noexcept {
		return std::forward_as_tuple( value.url_path, value.file_system_path,
		                              value.default_files, value.mime_db,
		                              value.ssl_config, value.port );
	}
} // namespace

int main( int argc, char const **argv ) {
	config_t config;
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

	auto site = config.ssl_config ? HttpSite( *config.ssl_config ) : HttpSite{};

	site
	  .on_listening( [&config]( EndPoint endpoint ) {
		  if( config.ssl_config ) {
			  std::cout << "Secure ";
		  }
		  std::cout << "Node++ Static HTTP Server\n";
		  std::cout << "Listening on " << endpoint << '\n';
	  } )
	  .on_error( []( base::Error error ) {
		  std::cerr << "Handling Error:\n";
		  std::cerr << error << '\n';
	  } )
	  .listen_on( config.port );

	auto service = HttpStaticService( config.url_path, config.file_system_path );
	service.connect( site );

	base::start_service( base::StartServiceMode::OnePerCore );
	return EXIT_SUCCESS;
}
