// The MIT License (MIT)
//
// Copyright (c) 2017-2018 Darrell Wright
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

#pragma once

#include <string>
#include <vector>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace file {
				struct FileInfo : public daw::json::daw_json_link<FileInfo> {
					struct FileData : public daw::json::daw_json_link<FileData> {
						std::string extension;
						std::string content_type;

						static void json_link_map( );
					}; // FileData

					std::vector<FileData> file_db;

					std::string get_content_type( daw::string_view path_string ) const;
					static void json_link_map( );
				}; // FileInfo

				std::string
				get_content_type( daw::string_view path_string,
				                  daw::string_view file_db_path = "./file_db.json" );
			} // namespace file
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
