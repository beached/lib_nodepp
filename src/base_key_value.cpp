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

#include "base_key_value.h"
#include <daw/daw_string_fmt.h>

namespace daw {
	namespace nodepp {
		namespace base {
			void key_value_t::json_link_map( ) {
				link_json_string( "key", key );
				link_json_string( "value", value );
			}

			key_value_t::key_value_t( std::string Key, std::string Value ) noexcept
			  : key( std::move( Key ) )
			  , value( std::move( Value ) ) {}

			std::string key_value_t::to_string( ) const {
				static const daw::fmt_t formatter = daw::fmt_t{"{0}:{1}"};
				return formatter( key, value );
			}

			bool key_value_t::empty( ) const noexcept {
				return key.empty( );
			}

			bool operator==( key_value_t const &lhs, key_value_t const &rhs ) {
				return ( lhs.key == rhs.key );
			}

			bool operator!=( key_value_t const &lhs, key_value_t const &rhs ) {
				return ( lhs.key == rhs.key );
			}

			bool operator<( key_value_t const &lhs, key_value_t const &rhs ) {
				return lhs.key < rhs.key;
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
