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

#include <daw/daw_string_view.h>

#include "base_event_emitter.h"

namespace daw {
	namespace nodepp {
		namespace base {
			StandardEventEmitter::StandardEventEmitter( size_t max_listeners )
			  : m_emitter( std::make_shared<emitter_t>( max_listeners ) ) {}

			void
			StandardEventEmitter::remove_all_callbacks( daw::string_view event ) {
				m_emitter->remove_all_callbacks( event );
			}

			size_t &StandardEventEmitter::max_listeners( ) noexcept {
				return m_emitter->max_listeners( );
			}

			size_t const &StandardEventEmitter::max_listeners( ) const noexcept {
				return m_emitter->max_listeners( );
			}

			size_t StandardEventEmitter::listener_count(
			  daw::string_view event_name ) const {
				return m_emitter->listener_count( event_name );
			}

			void StandardEventEmitter::emit_listener_added(
			  daw::string_view event,
			  StandardEventEmitter::callback_id_t callback_id ) {

				m_emitter->emit_listener_added( event, callback_id );
			}

			void StandardEventEmitter::emit_listener_removed(
			  daw::string_view event,
			  StandardEventEmitter::callback_id_t callback_id ) {

				m_emitter->emit_listener_removed( event, callback_id );
			}

			bool StandardEventEmitter::at_max_listeners( daw::string_view event ) {

				return m_emitter->at_max_listeners( event );
			}

			void StandardEventEmitter::emit_error( base::Error error ) {

				return m_emitter->emit( "error", daw::move( error ) );
			}

			void StandardEventEmitter::emit_error( std::string description,
			                                       std::string where ) {

				base::Error err{daw::move( description )};
				err.add( "where", daw::move( where ) );
				emit_error( daw::move( err ) );
			}

			void StandardEventEmitter::emit_error( base::Error const &child,
			                                       std::string description,
			                                       std::string where ) {

				base::Error err{daw::move( description )};
				err.add( "derived_error", "true" );
				err.add( "where", daw::move( where ) );
				err.add_child( child );
				emit_error( daw::move( err ) );
			}

			void StandardEventEmitter::emit_error( ErrorCode const &error,
			                                       std::string description,
			                                       std::string where ) {

				base::Error err{daw::move( description ), error};
				err.add( "where", daw::move( where ) );
				emit_error( daw::move( err ) );
			}

			void StandardEventEmitter::emit_error( std::exception_ptr ex,
			                                       std::string description,
			                                       std::string where ) {

				base::Error err{daw::move( description ), std::move( ex )};
				err.add( "where", daw::move( where ) );
				emit_error( daw::move( err ) );
			}

			bool StandardEventEmitter::is_same_instance(
			  StandardEventEmitter const &em ) const {

				return em.m_emitter == m_emitter;
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
