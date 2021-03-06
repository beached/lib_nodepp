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

#include <list>
#include <mutex>

#include <daw/daw_string_view.h>

#include "base_event_emitter.h"

namespace daw {
	namespace nodepp {
		namespace base {
			/// @brief Creates a class that will destruct after the event name passed
			/// to it is called(e.g. close/end) unless it is referenced elsewhere
			template<typename Derived, typename EventEmitter>
			class SelfDestructing
			  : public daw::nodepp::base::BasicStandardEvents<Derived, EventEmitter> {

				using daw::nodepp::base::BasicStandardEvents<Derived, EventEmitter>::get_ptr;
				using daw::nodepp::base::BasicStandardEvents<Derived, EventEmitter>::emitter;

				static std::list<
				  std::shared_ptr<SelfDestructing<Derived, EventEmitter>>> &
				s_selfs( ) noexcept {

					static auto result = std::list<
					  std::shared_ptr<SelfDestructing<Derived, EventEmitter>>>( );
					return result;
				}

				static std::mutex &s_mutex( ) noexcept {
					static std::mutex result{};
					return result;
				}

			public:
				explicit SelfDestructing(
				  daw::nodepp::base::StandardEventEmitter
				    emitter ) noexcept( is_nothrow_move_constructible_v<EventEmitter> )
				  : daw::nodepp::base::StandardEvents<Derived>( daw::move( emitter ) ) {
				}

				void arm( daw::string_view event_name ) {
					std::unique_lock<std::mutex> lock1( s_mutex( ) );
					auto obj = get_ptr( );
					auto pos = s_selfs( ).insert( s_selfs( ).end( ), obj );

					emitter( ).template add_listener<>(
					  event_name + "_selfdestruct",
					  [pos]( ) {
						  std::unique_lock<std::mutex> lock2( s_mutex( ) );
						  s_selfs( ).erase( pos );
					  },
					  true );
				}
			};
		} // namespace base
	}   // namespace nodepp
} // namespace daw
