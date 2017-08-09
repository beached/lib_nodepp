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

#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
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
				namespace impl {
					using namespace boost::asio::ip;
					using namespace daw::nodepp;

					NetDnsImpl::NetDnsImpl( base::EventEmitter emitter )
					    : daw::nodepp::base::StandardEvents<NetDnsImpl>{std::move( emitter )}
					    , m_resolver{std::make_unique<Resolver>( base::ServiceHandle::get( ) )} {}

					NetDnsImpl::~NetDnsImpl( ) = default;

					void NetDnsImpl::resolve( Resolver::query &query ) {
						m_resolver->async_resolve(
						    query, [obj = this->get_weak_ptr( )]( base::ErrorCode const &err, Resolver::iterator it ) {
							    handle_resolve( obj, err, std::move( it ) );
						    } );
					}

					void NetDnsImpl::resolve( daw::string_view address ) {
						auto query = tcp::resolver::query( address.to_string( ), "",
						                                   boost::asio::ip::resolver_query_base::numeric_host );
						resolve( query );
					}

					void NetDnsImpl::resolve( daw::string_view address, uint16_t port ) {
						auto query = tcp::resolver::query( address.to_string( ), std::to_string( port ),
						                                   boost::asio::ip::resolver_query_base::numeric_host );
						resolve( query );
					}

					NetDnsImpl &NetDnsImpl::on_resolved( std::function<void( Resolver::iterator )> listener ) {
						emitter( )->add_listener( "resolved", std::move( listener ) );
						return *this;
					}

					NetDnsImpl &NetDnsImpl::on_next_resolved( std::function<void( Resolver::iterator )> listener ) {
						emitter( )->add_listener( "resolved", std::move( listener ), true );
						return *this;
					}

					void NetDnsImpl::handle_resolve( std::weak_ptr<NetDnsImpl> obj, base::ErrorCode const &err,
					                                 Resolver::iterator it ) {
						run_if_valid( std::move( obj ), "Exception while resolving dns query", "NetDnsImpl::handle_resolve",
						              [&]( NetDns self ) {
							              if( !err ) {
								              self->emit_resolved( std::move( it ) );
							              } else {
								              self->emit_error( err, "NetDnsImpl::resolve" );
							              }
						              } );
					}

					void NetDnsImpl::emit_resolved( Resolver::iterator it ) {
						emitter( )->emit( "resolved", std::move( it ) );
					}

					NetDns NetDnsImpl::create( daw::nodepp::base::EventEmitter emitter ) {
						auto result = new NetDnsImpl{std::move( emitter )};
						return std::shared_ptr<NetDnsImpl>{result};
					}
				} // namespace impl

				NetDns create_net_dns( base::EventEmitter emitter ) {
					return impl::NetDnsImpl::create( std::move( emitter ) );
				}
			} // namespace net
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
