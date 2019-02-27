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

#pragma once

#include <daw/json/daw_json_link.h>

namespace daw {
	namespace nodepp {
		namespace base {
			struct key_value_t {
				std::string key{};
				std::string value{};

				key_value_t( ) noexcept = default;
				key_value_t( std::string Key, std::string Value ) noexcept;

				std::string to_string( ) const;

				bool empty( ) const noexcept;
			}; // key_value_t

			inline auto describe_json_class( key_value_t ) noexcept {
				using namespace daw::json;
				static constexpr char const k[] = "key";
				static constexpr char const v[] = "value";
				return class_description_t<json_string<k>, json_string<v>>{};
			}

			inline auto to_json_data( key_value_t const &kv ) noexcept {
				return std::forward_as_tuple( kv.key, kv.value );
			}

			bool operator==( key_value_t const &lhs, key_value_t const &rhs );
			bool operator!=( key_value_t const &lhs, key_value_t const &rhs );
			bool operator<( key_value_t const &lhs, key_value_t const &rhs );
		} // namespace base
	}   // namespace nodepp
} // namespace daw
