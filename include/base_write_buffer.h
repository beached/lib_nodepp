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

#include <asio/buffer.hpp>
#include <memory>

#include <daw/daw_string_view.h>

#include "base_memory.h"
#include "base_types.h"

namespace daw {
	namespace nodepp {
		namespace base {
			using MutableBuffer = asio::mutable_buffers_1;

			struct write_buffer {
				using data_type = base::data_t::pointer;
				base::shared_data_t buff;

				template<typename Iterator,
				         std::enable_if_t<( sizeof( typename std::iterator_traits<
				                                    Iterator>::value_type ) == 1 ),
				                          std::nullptr_t> = nullptr>
				write_buffer( Iterator first, Iterator last )
				  : buff{daw::nodepp::impl::make_shared_ptr<base::data_t>( first,
				                                                           last )} {}

				explicit write_buffer( base::data_t const &source );

				template<typename String,
				         std::enable_if_t<daw::traits::is_container_like_v<String>,
				                          std::nullptr_t> = nullptr>
				explicit write_buffer( String &source )
				  : write_buffer{std::cbegin( source ), std::cend( source )} {
					static_assert(
					  sizeof( decltype( *std::cbegin( source ) ) ),
					  "The source must be a container of byte sized values" );
				}

				std::size_t size( ) const noexcept;

				data_type data( ) const noexcept;
				MutableBuffer asio_buff( ) const;
			}; // struct write_buffer
		}    // namespace base
	}      // namespace nodepp
} // namespace daw
