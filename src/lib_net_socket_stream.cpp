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

#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <condition_variable>
#include <thread>

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>
#include <daw/daw_utility.h>

#include "base_enoding.h"
#include "base_error.h"
#include "base_event_emitter.h"
#include "base_selfdestruct.h"
#include "base_service_handle.h"
#include "base_stream.h"
#include "base_types.h"
#include "base_write_buffer.h"
#include "lib_net_dns.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using namespace daw::nodepp;
				using namespace boost::asio::ip;

				//////////////////////////////////////////////////////////////////////////
				/// Helpers
				///
				namespace impl {
					base::data_t get_clear_buffer( base::data_t &original_buffer,
					                               size_t num_items, size_t new_size ) {
						base::data_t new_buffer( new_size, 0 );
						using std::swap;
						swap( new_buffer, original_buffer );
						new_buffer.resize( num_items );
						return new_buffer;
					}
				} // namespace impl

				void set_ipv6_only( boost::asio::ip::tcp::acceptor &acceptor,
				                    ip_version ip_ver ) {
					if( ip_ver == ip_version::ipv4_v6 ) {
						// acceptor.set_option( boost::asio::ip::v6_only{false} );
						acceptor.set_option( boost::asio::ip::v6_only{true} );
					} else { // TODO verify correctness if( ip_ver == ip_version::ipv4_v6
						       // ) {
						// acceptor.set_option( boost::asio::ip::v6_only{true} );
						acceptor.set_option( boost::asio::ip::v6_only{false} );
					}
				}
			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
