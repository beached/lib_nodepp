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

#include <functional>

#include <daw/fs/function_stream.h>

namespace daw {
	namespace nodepp {
		namespace base {
			void on_main_thread( std::function<void( )> &&action );
			void on_main_thread( std::function<void( )> const &action );

			template<typename Task>
			auto add_task( Task task ) {
				auto ts = daw::get_task_scheduler( );
				if( !ts.started( ) ) {
					ts.start( );
				}
				ts.add_task( std::move( task ) );
			}

			template<typename Task, typename OnComplete>
			auto add_task( Task task, OnComplete on_complete ) {
				class on_complete_t {
					using TaskResult = std::decay_t<decltype( task( ) )>;
					OnComplete m_on_complete;

				public:
					explicit on_complete_t( OnComplete completer ) noexcept
					  : m_on_complete{std::move( completer )} {}

					void operator( )( TaskResult result ) const
					  noexcept( noexcept( on_complete ) ) {
						on_main_thread( [m_on_complete = this->m_on_complete,
						                 result = std::move( result )]( ) mutable {
							m_on_complete( std::move( result ) );
						} );
					}
				}; // on_complete_t
				auto fs = daw::make_function_stream( std::move( task ),
				                                     on_complete_t{on_complete} );
				return fs( );
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
