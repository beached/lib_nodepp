// The MIT License (MIT)
//
// Copyright (c) 2014-2016 Darrell Wright
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

#include <atomic>
#include <boost/any.hpp>
#include <functional>
#include <stdexcept>

#include <daw/daw_traits.h>
#include <daw/daw_utility.h>

namespace daw {
	namespace nodepp {
		namespace base {
			//////////////////////////////////////////////////////////////////////////
			// Summary:		CallbackImpl wraps a std::function or a c-style function ptr.
			//				This is needed because std::function are not comparable
			//				to each other.
			// Requires:
			struct Callback {
				using id_t = int64_t;
			private:
				static id_t get_last_id( ) noexcept;

				id_t m_id;
				boost::any m_callback;
			public:
				Callback( ) noexcept;

				~Callback( ) = default;

				template<typename Listener, typename = typename std::enable_if_t<!std::is_same<Listener, Callback>::value>>
					Callback( Listener listener ):
							m_id{ get_last_id( ) },
							m_callback{ daw::make_function( listener ) } { }

				Callback( Callback const & ) = default;

				Callback & operator=( Callback const & ) = default;

				Callback( Callback && ) = default;

				Callback & operator=( Callback && ) = default;

				id_t const & id( ) const noexcept;

				void swap( Callback & rhs ) noexcept;

				bool operator==( Callback const & rhs ) const noexcept;

				bool empty( ) const noexcept;

				template<typename... Args>
					void operator( )( Args && ... args ) {
						using cb_type = std::function<void( typename daw::traits::root_type_t<Args>... )>;
						try {
							auto callback = boost::any_cast<cb_type>( m_callback );
							callback( std::forward<Args>( args )... );
						} catch( boost::bad_any_cast const & ) {
							throw;
							//throw std::runtime_error( "Type of event listener does not match.  This shouldn't happen" );
						}
					}

			};    // Callback

			void swap( Callback & lhs, Callback & rhs ) noexcept;
		}    // namespace base
	}    // namespace nodepp
}    // namespace daw

