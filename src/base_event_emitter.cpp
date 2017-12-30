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
			  : m_emitter{daw::make_observable_ptr_pair<emitter_t>( max_listeners )} {}

			EventEmitter::~EventEmitter( ) = default;

			void EventEmitter::remove_all_callbacks( daw::string_view event ) {
				m_emitter.visit( [event]( emitter_t &em ) { em.remove_all_callbacks( event ); } );
			}

			size_t &EventEmitter::max_listeners( ) noexcept {
				return m_emitter.visit( []( emitter_t &em ) noexcept->size_t & { return em.max_listeners( ); } );
			}

			size_t const &EventEmitter::max_listeners( ) const noexcept {
				return m_emitter.visit( []( emitter_t const &em ) noexcept->size_t const & { return em.max_listeners( ); } );
			}

			size_t EventEmitter::listener_count( daw::string_view event_name ) const {
				return m_emitter.visit( [event_name]( emitter_t &em ) { return em.listener_count( event_name ); } );
			}

			void EventEmitter::emit_listener_added( daw::string_view event, EventEmitter::callback_id_t callback_id ) {
				m_emitter.visit( [event, callback_id]( emitter_t &em ) { em.emit_listener_added( event, callback_id ); } );
			}

			void EventEmitter::emit_listener_removed( daw::string_view event, EventEmitter::callback_id_t callback_id ) {
				m_emitter.visit( [event, callback_id]( emitter_t &em ) { em.emit_listener_removed( event, callback_id ); } );
			}

			bool EventEmitter::at_max_listeners( daw::string_view event ) {
				return m_emitter.visit( [event]( emitter_t &em ) { return em.at_max_listeners( event ); } );
			}

			void EventEmitter::emit_error( base::Error error ) {
				return m_emitter.visit( [error = std::move( error )]( emitter_t &em ) {
					return em.emit( "error", std::move( error ) );
				} );
			}

			void EventEmitter::emit_error( std::string description, std::string where ) {
				base::Error err{std::move( description )};
				err.add( "where", std::move( where ) );
				emit_error( std::move( err ) );
			}

			void EventEmitter::emit_error( base::Error const &child, std::string description, std::string where ) {
				base::Error err{std::move( description )};
				err.add( "derived_error", "true" );
				err.add( "where", std::move( where ) );
				err.add_child( child );
				emit_error( std::move( err ) );
			}

			void EventEmitter::emit_error( ErrorCode const &error, std::string description, std::string where ) {
				base::Error err{std::move( description ), error};
				err.add( "where", std::move( where ) );
				emit_error( std::move( err ) );
			}

			void EventEmitter::emit_error( std::exception_ptr ex, std::string description, std::string where ) {
				base::Error err{std::move( description ), std::move( ex )};
				err.add( "where", std::move( where ) );
				emit_error( std::move( err ) );
			}

			bool EventEmitter::is_same_instance( EventEmitter const & em ) const {
				auto rhs = em.m_emitter.borrow( );
				return m_emitter.apply_visitor( [&]( auto const & lhs ) {
					if( !lhs || !rhs ) {
						return false;
					}
					return lhs->is_same_instance( *rhs );
				});
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw

