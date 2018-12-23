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

#include <cstdint>
#include <string>

#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_string_view.h>

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace httpversion_impl {
					struct http_version_data_t {
						uint8_t minor;
						uint8_t major;
					};
					constexpr http_version_data_t
					parse_version_string( daw::string_view sv ) noexcept {
						sv = daw::parser::trim_left( sv );
						return {static_cast<uint8_t>( sv[0] - '0' ),
						        static_cast<uint8_t>( sv[2] - '0' )};
					}
				} // namespace httpversion_impl

				class HttpVersion {
					httpversion_impl::http_version_data_t m_data;

				public:
					explicit constexpr HttpVersion( daw::string_view version ) noexcept
					  : m_data( httpversion_impl::parse_version_string( version ) ) {}

					constexpr HttpVersion( uint8_t Major, uint8_t Minor ) noexcept
					  : m_data{Major, Minor} {}

					constexpr uint8_t version_major( ) const noexcept {
						return m_data.major;
					}

					constexpr uint8_t version_minor( ) const noexcept {
						return m_data.minor;
					}

					inline std::string to_string( ) const {
						std::string result{};
						result.resize( 3 );
						result[0] = static_cast<char>( '0' + version_major( ) );
						result[1] = '.';
						result[2] = static_cast<char>( '0' + version_major( ) );
						return result;
					}

					inline operator std::string( ) const {
						return to_string( );
					}
				};
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
