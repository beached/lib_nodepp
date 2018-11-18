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

#include <boost/filesystem.hpp>

#include <daw/daw_container_algorithm.h>
#include <daw/daw_string.h>

#include "lib_http.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					std::string find_host_name( HttpClientRequest const &request ) {
						auto host_it = request.headers.find( "Host" );
						if( request.headers.end( ) == host_it or
						    host_it->second.empty( ) ) {
							return std::string{};
						}
						return daw::string_view{host_it->second}
						  .pop_front( ":" )
						  .to_string( );
					}

					bool is_parent_of( boost::filesystem::path const &parent,
					                   boost::filesystem::path child ) {
						while( child.string( ).size( ) >= parent.string( ).size( ) ) {
							if( child == parent ) {
								return true;
							}
							child = child.parent_path( );
						}
						return false;
					}

					constexpr bool
					host_matches( daw::string_view const registered_host,
					              daw::string_view const current_host ) noexcept {
						return ( registered_host == current_host ) or
						       ( registered_host == "*" ) or ( current_host == "*" );
					}

					constexpr bool
					method_matches( HttpClientRequestMethod registered_method,
					                HttpClientRequestMethod current_method ) noexcept {
						return ( current_method == registered_method ) or
						       ( registered_method == HttpClientRequestMethod::Any ) or
						       ( current_method == HttpClientRequestMethod::Any );
					}
				} // namespace impl
			}   // namespace http
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
