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

#include <cstdint>
#include <string>
#include <sstream>
#include <utility>

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>

#include "lib_http_version.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				using namespace daw::nodepp;
				namespace {
					std::pair<uint8_t, uint8_t> parse_string( daw::string_view version ) {
						int ver_major, ver_minor;
						std::istringstream iss( version.to_string( ) );
						iss >> ver_major >> ver_minor; // TODO: fix, doesn't account for .
						                               // but assumes whitespace
						daw::exception::daw_throw_on_true<std::invalid_argument>(
						  ver_major < 0 || ver_major > std::numeric_limits<uint8_t>::max( ),
						  "Major version is out of range: " + version.to_string( ) );

						daw::exception::daw_throw_on_true<std::invalid_argument>(
						  ver_minor < 0 || ver_minor > std::numeric_limits<uint8_t>::max( ),
						  "Minor version is out of range: " + version.to_string( ) );

						return {ver_major, ver_minor};
					}
				} // namespace

				HttpVersion::HttpVersion( daw::string_view version ) noexcept
				  : HttpVersion{} {
					try {
						auto const p = parse_string( version );
						m_version_major = p.first;
						m_version_minor = p.second;
						m_is_valid = true;
					} catch( ... ) { m_is_valid = false; }
				}

				HttpVersion &HttpVersion::
				operator=( daw::string_view version ) noexcept {
					try {
						auto const p = parse_string( version );
						m_version_major = p.first;
						m_version_minor = p.second;
						m_is_valid = true;
					} catch( ... ) { m_is_valid = false; }
					return *this;
				}

				std::string HttpVersion::to_string( ) const {
					return std::to_string( version_major( ) ) + "." +
					       std::to_string( version_minor( ) );
				}

				HttpVersion::operator std::string( ) const {
					return to_string( );
				}
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
