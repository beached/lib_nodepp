// The MIT License (MIT)
//
// Copyright (c) 2014-2017 Darrell Wright
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

#include <asio/ip/tcp.hpp>
#include <cstdint>
#include <functional>
#include <memory>

#include <daw/daw_string_view.h>

#include "base_error.h"
#include "base_event_emitter.h"
#include "base_service_handle.h"
#include "lib_net_dns.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using namespace asio::ip;
				using namespace daw::nodepp;

				NetDns::NetDns( base::StandardEventEmitter &&emitter )
				  : daw::nodepp::base::StandardEvents<NetDns>( daw::move( emitter ) )
				  , m_resolver(
				      std::make_shared<Resolver>( base::ServiceHandle::get( ) ) ) {}

				void NetDns::resolve( Resolver::query &query ) {
					try {
						m_resolver->async_resolve(
						  query, [self = mutable_capture( *this )](
						           base::ErrorCode const &err, Resolver::iterator it ) {
							  handle_resolve( *self, err, daw::move( it ) );
						  } );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Error resolving DNS",
						            "NetDns::resolve" );
					}
				}

				void NetDns::resolve( daw::string_view address ) {
					try {
						auto query = tcp::resolver::query(
						  address.to_string( ), "",
						  asio::ip::resolver_query_base::numeric_host );
						resolve( query );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Error resolving DNS",
						            "NetDns::resolve" );
					}
				}

				void NetDns::resolve( daw::string_view address, uint16_t port ) {
					try {
						auto query = tcp::resolver::query(
						  address.to_string( ), std::to_string( port ),
						  asio::ip::resolver_query_base::numeric_host );
						resolve( query );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Error resolving DNS",
						            "NetDns::resolve" );
					}
				}

				void NetDns::handle_resolve( NetDns self, base::ErrorCode const &err,
				                             Resolver::iterator it ) {
					try {
						if( !err ) {
							self.emit_resolved( daw::move( it ) );
						} else {
							self.emit_error( err, "Exception while resolving dns query",
							                 "NetDns::resolve" );
						}
					} catch( ... ) {
						self.emit_error( std::current_exception( ),
						                 "Exception while resolving dns query",
						                 "NetDns::handle_resolve" );
					}
				}

				void NetDns::emit_resolved( Resolver::iterator it ) {
					emitter( ).emit( "resolved", daw::move( it ) );
				}
			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
