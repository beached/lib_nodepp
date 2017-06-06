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

#include "lib_net_server.h"

struct config_t : public daw::json::JsonLink<config_t> {
	uint16_t port;

	config_t( ) : daw::json::JsonLink<config_t>{}, port{12345} {

		link_values( );
	}

	config_t( config_t const &other ) : daw::json::JsonLink<config_t>{}, port{other.port} {

		link_values( );
	}

	config_t( config_t &&other ) : daw::json::JsonLink<config_t>{}, port{std::move( other.port )} {

		link_values( );
	}

	config_t &operator=( config_t const &rhs ) {
		if( this != &rhs ) {
			using std::swap;
			config_t tmp{rhs};
			swap( *this, tmp );
		}
		return *this;
	}

	config_t &operator=( config_t &&rhs ) {
		if( this != &rhs ) {
			using std::swap;
			config_t tmp{rhs};
			swap( *this, tmp );
		}
		return *this;
	}

	~config_t( );

  private:
	void link_values( ) {
		this->link_integral( "port", port );
	}
}; // config_t

config_t::~config_t( ) {}

int main( int argc, char const **argv ) {
	config_t config;
	if( argc > 1 ) {
		try {
			config.from_file( argv[1] );
		} catch( std::exception const & ) {
			std::cerr << "Error parsing config file" << std::endl;
			exit( EXIT_FAILURE );
		}
	} else {
		std::string fpath = argv[0];
		fpath += ".json";
		config.to_file( fpath );
	}

	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;

	auto server = create_net_server( );

	server
	    ->on_connection( [&]( auto socket ) {
		    socket << "Goodbye";
		    socket->on_all_writes_completed( [socket]( auto ) { socket->close( ); } );
	    } )
	    .on_listening( []( auto endpoint ) { std::cout << "listening on " << endpoint << '\n'; } )
	    .listen( config.port );

	base::start_service( base::StartServiceMode::Single );
	return EXIT_SUCCESS;
}
