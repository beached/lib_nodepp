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

#include <daw/daw_container_algorithm.h>
#include <daw/daw_string_fmt.h>
#include <daw/daw_string_view.h>

#include "base_error.h"

namespace daw {
	namespace nodepp {
		namespace base {
			Error::Error( std::string description ) {
				add( "description", daw::move( description ) );
			}

			Error::Error( std::string description, ErrorCode const &err ) {
				add( "description", daw::move( description ) );
				add( "message", err.message( ) );
				add( "category", std::string{err.category( ).name( )} );
				add( "error_code", std::to_string( err.value( ) ) );
			}

			Error::Error( std::string description, std::exception_ptr ex_ptr ) {
				add( "description", daw::move( description ) );
				m_exception = daw::move( ex_ptr );
			}

			Error &Error::add( std::string name, std::string value ) {
				daw::exception::precondition_check(
				  !m_frozen, "Attempt to change a frozen Error." );

				m_key_values.emplace_back( daw::move( name ), std::move( value ) );
				return *this;
			}

			daw::string_view Error::get( daw::string_view name ) const {
				auto pos = daw::container::find_if(
				  m_key_values, [name]( auto const &current_value ) {
					  return current_value.key == name;
				  } );

				daw::exception::precondition_check<std::out_of_range>(
				  pos != m_key_values.cend( ),
				  "Name does not exist in Error key values" );
				return pos->value;
			}

			void Error::freeze( ) {
				m_frozen = true;
			}

			Error const &Error::child( ) const {
				return *m_child;
			}

			bool Error::has_child( ) const {
				return static_cast<bool>( m_child );
			}

			bool Error::has_exception( ) const {
				return static_cast<bool>( m_exception ) or
				       ( has_child( ) and child( ).has_exception( ) );
			}

			void Error::throw_exception( ) {
				if( has_child( ) and child( ).has_exception( ) ) {
					m_child->throw_exception( );
				}
				if( has_exception( ) ) {
					auto current_exception = std::exchange( m_exception, nullptr );
					std::rethrow_exception( current_exception );
				}
			}

			void Error::add_child( Error const &child ) {
				daw::exception::precondition_check(
				  !m_frozen, "Attempt to change a frozen Error." );
				freeze( );
				m_child = daw::make_copiable_unique_ptr<Error>( child );
			}

			std::string Error::to_string( std::string const &prefix ) const {
				/*
				if( !daw::container::contains(
				      m_key_values, []( auto const &current_value ) { return
				current_value.key == "description"; } ) ) {

				  return daw::fmt( "{0}Error: Error in error, missing description\n",
				prefix );
				}
				 */
				std::stringstream ss;
				daw::fmt_t kv_fmt{prefix + "'{0}',	'{1}'\n"};
				for( auto const &row : m_key_values ) {
					ss << kv_fmt( row.key, row.value );
				}
				if( m_exception ) {
					try {
						std::rethrow_exception( m_exception );
					} catch( std::exception const &ex ) {
						ss << daw::fmt( "Exception message: {0}\n", ex.what( ) );
					} catch( Error const &err ) {
						ss << daw::fmt( "Exception message: {0}\n", err );
					} catch( ... ) { ss << "Unknown exception\n"; }
				}
				if( has_child( ) ) {
					ss << child( ).to_string( daw::fmt( "{0}#", prefix ) );
				}
				ss << '\n';
				return ss.str( );
			}

			std::string Error::to_string( ) const {
				return to_string( "" );
			}

			std::ostream &operator<<( std::ostream &os, Error const &error ) {
				os << error.to_string( );
				return os;
			}

			std::optional<Error> create_optional_error( ) {
				return std::optional<Error>( );
			}
		} // namespace base
	}   // namespace nodepp
} // namespace daw
