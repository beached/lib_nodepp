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

#include <future>

#include <daw/daw_exception.h>

#include "base_service_handle.h"

namespace daw {
	namespace nodepp {
		namespace base {
			IoService &ServiceHandle::get( ) {
				static IoService result{};
				return result;
			}

			void ServiceHandle::run( ) {
				get( ).run( );
			}

			void ServiceHandle::stop( ) {
				get( ).stop( );
			}

			void ServiceHandle::reset( ) {
				get( ).reset( );
			}

			void ServiceHandle::work( ) {
				IoService::work work( get( ) );
			}

			void start_service( daw::nodepp::base::StartServiceMode mode ) {
				switch( mode ) {
				case StartServiceMode::Single:
					ServiceHandle::run( );
					break;
				case StartServiceMode::OnePerCore:
					for( size_t n = 1; n < std::thread::hardware_concurrency( ); ++n ) {
						std::async( []( ) { ServiceHandle::run( ); } );
					}
					ServiceHandle::run( );
					break;
				default:
					daw::exception::daw_throw_unexpected_enum( );
				}
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
