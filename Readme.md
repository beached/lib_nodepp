A high level library to let me do asynchronous c++ programming like that done in Node.js.
```C++
#include <cstdlib>

#include "base_work_queue.h"
#include "lib_http_request.h"
#include "lib_http_site.h"
#include "lib_http_static_service.h"

int main( int argc, char const **argv ) {
	using namespace daw::nodepp;
	using namespace daw::nodepp::lib::net;
	using namespace daw::nodepp::lib::http;

	auto site = create_http_site( );
	site->on_listening( []( EndPoint endpoint ) {
				std::cout << "Node++ Static HTTP Server\n";
				std::cout << "Listening on " << endpoint << '\n';
			} )
			.on_error( []( base::Error error ) {
				std::cerr << "Error: ";
				std::cerr << error << '\n';
			} )
			.listen_on( 8080 );

	auto service = create_static_service( "/", "./web_files" );
	service->connect( site );

	base::start_service( base::StartServiceMode::OnePerCore );
	return EXIT_SUCCESS;
}
```
As you can see, one can create a static web server quickly and succinctly. Currently, the library focus is on networking and HTTP networking. Other async tasks will be added, such as Sqlite support as I need them.  The tests folder contains more examples.
