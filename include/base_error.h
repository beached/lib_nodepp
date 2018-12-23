// The MIT License (MIT)
//
// Copyright (c) 2014-2018 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
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

#include <memory>
#include <optional>
#include <system_error>

#include <daw/daw_copiable_unique_ptr.h>
#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

#include "base_key_value.h"

namespace daw {
	namespace nodepp {
		namespace base {
			using ErrorCode = std::error_code;

			/// Contains key/value pairs describing an error condition.
			class Error {
				std::vector<key_value_t> m_key_values{};
				daw::copiable_unique_ptr<Error> m_child{};
				std::exception_ptr m_exception = nullptr;
				bool m_frozen = false;

			public:
				explicit Error( std::string description );
				explicit Error( std::string description, ErrorCode const &err );
				Error( std::string description, std::exception_ptr ex_ptr );

				Error &add( std::string name, std::string value );
				daw::string_view get( daw::string_view name ) const;

				Error const &child( ) const;
				bool has_child( ) const;
				void add_child( Error const &child );
				void freeze( );
				bool has_exception( ) const;
				void throw_exception( );
				std::string to_string( std::string const &prefix ) const;
				std::string to_string( ) const;
			}; // class Error

			std::ostream &operator<<( std::ostream &os, Error const &error );

			//////////////////////////////////////////////////////////////////////////
			/// @brief	Create a null error (e.g. no error)
			std::optional<Error> create_optional_error( );

			//////////////////////////////////////////////////////////////////////////
			/// @brief	Create an error item
			template<typename... Args>
			std::optional<Error> create_optional_error( Args &&... args ) {
				return Error( std::forward<Args>( args )... );
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
