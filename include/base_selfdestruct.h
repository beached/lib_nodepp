// The MIT License (MIT)
//
// Copyright (c) 2014-2017 Darrell Wright
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

#pragma once

#include <list>
#include <mutex>

#include <daw/daw_string_view.h>

#include "base_event_emitter.h"

namespace daw {
	namespace nodepp {
		namespace base {
			// Creates a class that will destruct after the event name passed to it is called(e.g. close/end) unless it
			// is referenced elsewhere
			template<typename Derived>
			class SelfDestructing : public daw::nodepp::base::enable_shared<Derived>,
			                        public daw::nodepp::base::StandardEvents<Derived> {

				static std::list<std::shared_ptr<SelfDestructing<Derived>>> &s_selfs( ) noexcept {
					static std::list<std::shared_ptr<SelfDestructing<Derived>>> result;
					return result;
				}

				static std::mutex &s_mutex( ) noexcept {
					static std::mutex result;
					return result;
				}

			  public:
				SelfDestructing( ) = delete;

				explicit SelfDestructing( daw::nodepp::base::EventEmitter emitter )
				    : daw::nodepp::base::StandardEvents<Derived>{std::move( emitter )} {}

				~SelfDestructing( ) = default;
				SelfDestructing( SelfDestructing const & ) = default;
				SelfDestructing( SelfDestructing && ) noexcept = default;
				SelfDestructing &operator=( SelfDestructing const & ) = default;
				SelfDestructing &operator=( SelfDestructing && ) noexcept = default;

				void arm( daw::string_view event ) {
					std::unique_lock<std::mutex> lock1{s_mutex( )};
					auto obj = this->get_ptr( );
					auto pos = s_selfs( ).insert( s_selfs( ).end( ), obj );
					this->emitter( )->add_listener( event + "_selfdestruct",
					                                [pos]( ) {
						                                std::unique_lock<std::mutex> lock2( s_mutex( ) );
						                                s_selfs( ).erase( pos );
					                                },
					                                true );
				}
			}; // class SelfDestructing
		}      // namespace base
	}          // namespace nodepp
} // namespace daw
