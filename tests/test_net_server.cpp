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
#include <string>

#include <daw/daw_read_file.h>
#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_link_file.h>

#include "lib_net_server.h"

namespace {
	struct config_t {
		uint16_t port = 12345U;
	};

	inline auto describe_json_class( config_t ) noexcept {
		using namespace daw::json;
		static constexpr char const n0[] = "port";
		return class_description_t<json_number<n0, uint16_t>>{};
	}

	constexpr inline auto to_json_data( config_t const &value ) noexcept {
		return std::forward_as_tuple( value.port );
	}
} // namespace

int main( int argc, char const **argv ) {
	auto config = config_t( );
	if( argc > 1 ) {
		try {
			auto const json_data = daw::read_file( argv[1] );
			config = daw::json::from_json<config_t>( json_data );
		} catch( std::exception const & ) {
			std::cerr << "Error parsing config file" << std::endl;
			exit( EXIT_FAILURE );
		}
	} else {
		std::string fpath = argv[0];
		fpath += ".json";
		// TODO config.to_file( fpath );
	}

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;

	auto server = NetServer( );

	server.on_connection( [&]( NetServerSocket socket ) {
		auto const remote_info =
		  socket.remote_address( ) + std::to_string( socket.remote_port( ) );
		std::cout << "Connection open: " << remote_info << '\n';

		// TODO: this is bad
		socket.on_data_received(
		  [socket = daw::mutable_capture( daw::move( socket ) )](
		    std::shared_ptr<base::data_t> buffer, bool eof ) {
			  bool has_eof_marker = false;
			  if( buffer ) {
				  auto sv = daw::string_view( buffer->data( ), buffer->size( ) );
				  has_eof_marker = sv.find_first_of( 0x04 ) != sv.npos;
				  std::cout << "Recv: " << sv << '\n';
			  }
			  if( eof or has_eof_marker ) {
				  return;
			  }
			  socket->read_async( );
		  } );

		socket.on_closed( [remote_info]( ) {
			std::cout << "Connection closed: " << remote_info << '\n';
		} );

		socket << "Hello" << eol << eol;
		socket.read_async( );
	} );

	server.on_listening( []( lib::net::EndPoint endpoint ) {
		std::cout << "listening on " << endpoint << '\n';
	} );

	server.on_error( []( base::Error err ) {
		std::cerr << "Error:" << err;
		std::cerr << std::endl;
	} );
	server.listen( config.port );

	base::start_service( base::StartServiceMode::Single );
	return EXIT_SUCCESS;
}
