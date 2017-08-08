A high level library to let me do asynchronous c++ programming like that done in Node.js.
```C++
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_link_file.h>

struct config_t : public daw::json::daw_json_link<config_t> {
  uint16_t port;
  std::string url_path;
  std::string file_system_path;
  std::vector<std::string> default_files;

  config_t( ) : port{8080}, url_path{"/"}, file_system_path{"./web_files"}, default_files{} {}

  static void json_link_map( ) {
    link_json_integer( "port", port );
    link_json_string( "url_path", url_path );
    link_json_string( "file_system_path", file_system_path );
    link_json_string_array( "default_files", default_files );
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

  return EXIT_SUCCESS;
}
```
As you can see, one can create a static web server quickly and succinctly. Currently, the library focus is on networking and HTTP networking. Other async tasks will be added, such as Sqlite support as I need them
