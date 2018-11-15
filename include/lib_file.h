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

#pragma once

#include <mutex>
#include <type_traits>

#include <daw/daw_string_view.h>

#include "base_event_emitter.h"
#include "base_stream.h"
#include "base_task_management.h"
#include "base_write_buffer.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace file {
				using namespace daw::nodepp;

				std::streampos file_size( daw::string_view path );

				//////////////////////////////////////////////////////////////////////////
				/// @brief	Reads in contents of file and appends it to buffer
				base::OptionalError read_file( daw::string_view path,
				                               base::data_t &buffer,
				                               bool append_buffer = true );

				template<typename Callback>
				void read_file_async( daw::string_view path, Callback &&on_completion,
				                      std::shared_ptr<base::data_t> buffer = nullptr,
				                      bool append_buffer = true ) {

					static_assert( std::is_invocable_v<Callback, base::OptionalError,
					                                  std::shared_ptr<base::data_t>>,
					               "Callback does not accept required arguments" );

					auto task = [path, buffer, append_buffer]( ) mutable {
						if( !buffer ) {
							buffer.reset( new base::data_t{} );
						} else if( !append_buffer ) {
							buffer->resize( 0 );
						}
						return read_file( path, *buffer );
					};

					base::add_task( std::move( task ),
					                std::forward<Callback>( on_completion ) );
				}

				enum class FileWriteMode : uint_fast8_t {
					OverwriteOrCreate,
					AppendOrCreate,
					MustCreate
				};

				base::OptionalError
				write_file( daw::string_view path, base::data_t const &buffer,
				            FileWriteMode mode = FileWriteMode::MustCreate,
				            size_t bytes_to_write = 0 );

				template<typename Callback>
				void write_file_async( daw::string_view path, base::data_t buffer,
				                       Callback &&on_completion,
				                       FileWriteMode mode = FileWriteMode::MustCreate,
				                       size_t bytes_to_write = 0 ) {

					static_assert( std::is_invocable_v<Callback, base::OptionalError>,
					               "Callback does not accept requried arguments" );

					auto task = [path, buffer = std::move( buffer ), mode,
					             bytes_to_write]( ) mutable {
						return write_file( path, buffer, mode, bytes_to_write );
					};

					base::add_task( task, std::forward<Callback>( on_completion ) );
				}

			} // namespace file
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
