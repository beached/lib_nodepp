// The MIT License (MIT)
//
// Copyright (c) 2014-2016 Darrell Wright
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

#include <boost/utility/string_view.hpp>
#include <boost/system/error_code.hpp>
#include <map>
#include <memory>
#include <daw/json/daw_json_link.h>
#include <daw/daw_optional_poly.h>

namespace daw {
	namespace nodepp {
		namespace base {
			using ErrorCode = ::boost::system::error_code;

			//////////////////////////////////////////////////////////////////////////
			// Summary:		Contains key/value pairs describing an error condition.
			//				Description is mandatory.
			// Requires:
			class Error: public std::exception, public daw::json::JsonLink<Error> {
				std::map<std::string, std::string> m_keyvalues;
				bool m_frozen;
				daw::optional_poly<Error> m_child;
				std::exception_ptr m_exception;
				void set_links( );
				Error( ) = default;
				friend class daw::json::JsonLink<Error>;
			public:
				~Error( );

				explicit Error( boost::string_view description );

				explicit Error( ErrorCode const &err );

				Error( Error const & other );
				Error( Error && other );
				Error & operator=( Error const & rhs );
				Error & operator=( Error && rhs ) noexcept;
				void swap( Error & rhs ) noexcept;

				Error( boost::string_view description, std::exception_ptr ex_ptr );

				Error &add( boost::string_view name, boost::string_view value );

				boost::string_view get( boost::string_view name ) const;

				std::string &get( boost::string_view name );

				Error const & child( ) const;
				Error & child( );

				bool has_child( ) const;

				Error &clear_child( );

				Error &child( Error child );

				void freeze( );

				bool has_exception( ) const;

				void throw_exception( );

				std::string to_string( boost::string_view prefix = "" ) const;
			};    // class Error

			void swap( Error & lhs, Error & rhs ) noexcept;

			std::ostream &operator<<( std::ostream &os, Error const &error );

			using OptionalError = daw::optional_poly<Error>;

			//////////////////////////////////////////////////////////////////////////
			/// Summary:	Create a null error (e.g. no error)
			OptionalError create_optional_error( );

			//////////////////////////////////////////////////////////////////////////
			/// Summary:	Create an error item
			template<typename... Args>
			OptionalError create_optional_error( Args &&... args ) {
				Error err{ std::forward<Args>( args )... };
				return OptionalError{ std::move( err ) };
			}
		}    // namespace base
	}    // namespace nodepp
}    // namespace daw

