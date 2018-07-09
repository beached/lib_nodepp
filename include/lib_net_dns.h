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

#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <functional>

#include <daw/daw_string_view.h>
#include <daw/daw_utility.h>

#include "base_event_emitter.h"
#include "base_service_handle.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using Resolver = boost::asio::ip::tcp::resolver;

				class NetDns : public daw::nodepp::base::StandardEvents<NetDns> {
					std::shared_ptr<Resolver> m_resolver;
					static void handle_resolve( NetDns self, base::ErrorCode const &err,
					                            Resolver::iterator it );

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when async resolve is complete
					void emit_resolved( Resolver::iterator it );

				public:
					explicit NetDns( daw::nodepp::base::StandardEventEmitter &&emitter =
					                   daw::nodepp::base::StandardEventEmitter{} );

					using handler_argument_t = Resolver::iterator;

					/// @brief resolve name or ip address and call callback of form
					/// void(base::ErrorCode, Resolver::iterator)
					void resolve( daw::string_view address );
					void resolve( daw::string_view address, uint16_t port );
					void resolve( Resolver::query &query );

					//////////////////////////////////////////////////////////////////////////
					// Event callbacks

					/// @brief Event emitted when name resolution is complete
					template<typename Listener>
					NetDns &on_resolved( Listener &&listener ) {
						emitter( ).template add_listener<Resolver::iterator>(
						  "resolved", std::forward<Listener>( listener ) );
						return *this;
					}

					/// @brief Event emitted when name resolution is complete
					template<typename Listener>
					NetDns &on_next_resolved( Listener &&listener ) {
						emitter( ).template add_listener<Resolver::iterator>(
						  "resolved", std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}
				}; // class NetDns
			}    // namespace net
		}      // namespace lib
	}        // namespace nodepp
} // namespace daw
