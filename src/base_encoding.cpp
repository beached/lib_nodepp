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

#include <stdexcept>
#include <string>
#include <vector>

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>

#include "base_enoding.h"

namespace daw {
	namespace nodepp {
		namespace base {
			std::vector<std::string> const &Encoding::valid_encodings( ) {
				static std::vector<std::string> const result = {
				  "ascii", "utf8", "utf16le", "ucs2", "hex"};
				return result;
			}

			Encoding::Encoding( std::string encoding )
			  : m_encoding{daw::move( encoding )} {}

			Encoding &Encoding::operator=( daw::string_view rhs ) {
				daw::exception::precondition_check( is_valid_encoding( rhs ),
				                                    "Encoding is not valid" );

				m_encoding = rhs.to_string( );
				return *this;
			}

			daw::string_view Encoding::operator( )( ) const {
				return m_encoding;
			}

			void Encoding::set( std::string encoding ) {
				daw::exception::precondition_check( is_valid_encoding( encoding ),
				                                    "Encoding is not valid" );

				m_encoding = encoding;
			}

			bool Encoding::is_valid_encoding( daw::string_view enc ) {
				auto const &encs = valid_encodings( );
				return std::find( encs.cbegin( ), encs.cend( ), enc.to_string( ) ) !=
				       encs.cend( );
			}

			Encoding::operator std::string( ) const {
				return m_encoding;
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
