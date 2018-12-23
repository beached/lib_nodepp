// The MIT License (MIT)
//
// Copyright (c) 2018 Darrell Wright
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
#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_link_file.h>

#include "lib_file_info.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace file {
				void FileInfo::FileData::json_link_map( ) {
					link_json_string( "extension", extension );
					link_json_string( "content-type", content_type );
				}

				void FileInfo::json_link_map( ) {
					link_json_object_array( "file_db", file_db );
				}

				std::string
				FileInfo::get_content_type( daw::string_view path_string ) const {
					boost::filesystem::path const path{path_string};
					auto ext = extension( path );
					if( ext.empty( ) ) {
						return ext;
					}
					ext = ext.substr( 1 ); // remove prefixed dot(.)

					// TODO: C++ 17
					auto it = daw::container::find_if( file_db, [&ext]( auto const &fi ) {
						// TODO: handle file case
						return fi.extension == ext;
					} );
					if( it == file_db.cend( ) ) {
						return std::string{};
					}
					return it->content_type;
				}

				std::string get_content_type( daw::string_view path_string,
				                              daw::string_view file_db_path ) {
					static auto const &s_file_db =
					  daw::json::from_file<FileInfo>( file_db_path );
					return s_file_db.get_content_type( path_string );
				}
			} // namespace file
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
