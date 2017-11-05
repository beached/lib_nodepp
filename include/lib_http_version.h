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

#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include <daw/daw_string_view.h>

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				class HttpVersion {
					uint_fast8_t m_version_major;
					uint_fast8_t m_version_minor;
					bool m_is_valid;

				public:
					constexpr HttpVersion( ) noexcept : m_version_major{0}, m_version_minor{0}, m_is_valid{false} {}

					~HttpVersion( ) = default;

					constexpr HttpVersion( HttpVersion const & ) = default;
					constexpr HttpVersion( HttpVersion && ) noexcept = default;
					constexpr HttpVersion &operator=( HttpVersion const & ) noexcept = default;
					constexpr HttpVersion &operator=( HttpVersion &&rhs ) noexcept = default;

					HttpVersion &operator=( daw::string_view version ) noexcept;
					explicit HttpVersion( daw::string_view version ) noexcept;

					constexpr HttpVersion( uint_fast8_t Major, uint_fast8_t Minor ) noexcept
					  : m_version_major{Major}, m_version_minor{Minor}, m_is_valid{true} {}

					constexpr uint_fast8_t const &version_major( ) const noexcept {
						return m_version_major;
					}

					constexpr uint_fast8_t &version_major( ) noexcept {
						return m_version_major;
					}

					constexpr uint_fast8_t const &version_minor( ) const noexcept {
						return m_version_minor;
					}

					constexpr uint_fast8_t &version_minor( ) noexcept {
						return m_version_minor;
					}

					constexpr bool is_valid( ) const noexcept {
						return m_is_valid;
					}

					std::string to_string( ) const;
					explicit operator std::string( ) const;
				};
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
