// The MIT License (MIT)
//
// Copyright (c) 2014-2017 Darrell Wright
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
// all
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

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <map>
#include <memory>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

namespace daw {
	namespace nodepp {
		namespace base {
			using ErrorCode = ::boost::system::error_code;

			//////////////////////////////////////////////////////////////////////////
			// Summary:		Contains key/value pairs describing an error condition.
			//				Description is mandatory.
			// Requires:
			// DAW class Error : public std::exception, public daw::json::daw_json_link<Error> {
			class Error {
				std::vector<std::pair<std::string, std::string>> m_keyvalues;
				bool m_frozen;
				std::unique_ptr<Error> m_child;
				std::exception_ptr m_exception;

			  public:
				Error( ) = delete;

				explicit Error( daw::string_view description );
				explicit Error( daw::string_view description, ErrorCode const &err );
				Error( daw::string_view description, std::exception_ptr ex_ptr );
				~Error( );
				Error( Error &&other ) noexcept = default;
				Error &operator=( Error &&rhs ) noexcept = default;

				Error( Error const &other );
				Error &operator=( Error const &rhs );

				Error &add( daw::string_view name, daw::string_view value );
				daw::string_view get( daw::string_view name ) const;

				Error const &child( ) const;
				bool has_child( ) const;
				void add_child( Error const &child );
				void freeze( );
				bool has_exception( ) const;
				void throw_exception( );
				std::string to_string( daw::string_view prefix = "" ) const;
			}; // class Error

			using OptionalError = boost::optional<Error>;

			std::ostream &operator<<( std::ostream &os, Error const &error );

			//////////////////////////////////////////////////////////////////////////
			/// Summary:	Create a null error (e.g. no error)
			OptionalError create_optional_error( );

			//////////////////////////////////////////////////////////////////////////
			/// Summary:	Create an error item
			template<typename... Args>
			OptionalError create_optional_error( Args &&... args ) {
				Error err{std::forward<Args>( args )...};
				return OptionalError{std::move( err )};
			}
		} // namespace base
	}     // namespace nodepp
} // namespace daw
