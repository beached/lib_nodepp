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

#include <asio/streambuf.hpp>
#include <functional>

#include "base_event_emitter.h"
#include "base_types.h"

namespace daw {
	namespace nodepp {
		namespace base {
			namespace stream {
				using StreamBuf = asio::streambuf;

				template<typename Derived>
				class StreamWritableEvents {
					Derived &derived( ) noexcept {
						return *static_cast<Derived *>( this );
					}

					Derived const &derived( ) const noexcept {
						return *static_cast<Derived const *>( this );
					}

					decltype( auto ) derived_emitter( ) noexcept(
					  noexcept( std::declval<Derived>( ).emitter( ) ) ) {
						return derived( ).emitter( );
					}

					decltype( auto ) derived_emitter( ) const
					  noexcept( noexcept( std::declval<Derived>( ).emitter( ) ) ) {
						return derived( ).emitter( );
					}

				public:
					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when a pending write is completed
					template<typename Listener>
					decltype( auto ) on_write_completion( Listener &&listener ) {
						derived_emitter( ).template add_listener<Derived>(
						  "write_completion", std::forward<Listener>( listener ) );
						return derived( );
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when the next pending write is completed
					template<typename Listener>
					decltype( auto ) on_next_write_completion( Listener &&listener ) {
						derived_emitter( ).template add_listener<Derived>(
						  "write_completion", std::forward<Listener>( listener ),
						  Derived::callback_runmode_t::run_once );
						return derived( );
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when end( ... ) has been called and all
					///				data has been flushed
					template<typename Listener>
					decltype( auto ) on_all_writes_completed( Listener &&listener ) {
						derived_emitter( ).template add_listener<Derived>(
						  "all_writes_completion", std::forward<Listener>( listener ) );
						return derived( );
					}

					decltype( auto ) close_when_writes_completed( ) {
						on_all_writes_completed(
						  []( Derived resp ) { resp.close( false ); } );
						return derived( );
					}

					/// @brief	Event emitted when an async write completes
					void emit_write_completion( Derived &obj ) {
						derived_emitter( ).emit( "write_completion", obj );
					}

					/// @brief	All async writes have completed
					void emit_all_writes_completed( Derived &obj ) {
						derived_emitter( ).emit( "all_writes_completed", obj );
					}
				}; // class StreamWritableEvents
			}    // namespace stream
		}      // namespace base
	}        // namespace nodepp
} // namespace daw
