// The MIT License (MIT)
//
// Copyright (c) 2017 Darrell Wright
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

#include <daw/daw_string_view.h>

#include "base_event_emitter.h"

namespace daw {
	namespace nodepp {
		namespace base {
			EventEmitter::EventEmitter( size_t max_listeners )
			  : m_emitter{daw::make_observable_ptr<emitter_t>( max_listeners )} {}

			EventEmitter::EventEmitter( EventEmitter const &other )
			  : m_emitter{impl::get_observer( other.m_emitter )} {}

			EventEmitter &EventEmitter::operator=( EventEmitter const &rhs ) {
				if( this != &rhs ) {
					m_emitter = impl::get_observer( rhs.m_emitter );
				}
				return *this;
			}

			EventEmitter::~EventEmitter( ) = default;

			void EventEmitter::remove_all_callbacks( daw::string_view event ) {
				impl::with_observer( m_emitter, [event]( auto em ) -> void { em->remove_all_callbacks( event ); } );
			}

			size_t &EventEmitter::max_listeners( ) noexcept {
				return impl::with_observer( m_emitter, []( auto em ) noexcept->size_t &
				                                         { return em->max_listeners( ); } );
			}

			size_t const &EventEmitter::max_listeners( ) const noexcept {
				return impl::with_const_observer(
				  m_emitter, []( auto em ) noexcept->size_t const & { return em->max_listeners( ); } );
			}

			size_t EventEmitter::listener_count( daw::string_view event_name ) const {
				return impl::with_const_observer( m_emitter,
				                            [event_name]( auto em ) { return em->listener_count( event_name ); } );
			}

			void EventEmitter::emit_listener_added( daw::string_view event, EventEmitter::callback_id_t callback_id ) {
				impl::with_observer( m_emitter,
				                     [event, callback_id]( auto em ) { em->emit_listener_added( event, callback_id ); } );
			}

			void EventEmitter::emit_listener_removed( daw::string_view event, EventEmitter::callback_id_t callback_id ) {
				impl::with_observer(
				  m_emitter, [event, callback_id]( auto em ) { em->emit_listener_removed( event, callback_id ); } );
			}

			bool EventEmitter::at_max_listeners( daw::string_view event ) {
				return impl::with_observer( m_emitter, [event]( auto em ) { return em->at_max_listeners( event ); } );
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
