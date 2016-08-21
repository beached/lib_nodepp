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

#include <exception>

#include <daw/daw_traits.h>

namespace daw {
	namespace parser {
		template<typename T, typename Iterator>
			struct parser_result {
				T result;
				Iterator first;
				Iterator last;
			};    // parser_result

		struct ParserException: public std::exception {
			virtual ~ParserException( ) = default;
		};    // ParserException

		struct ParserEmptyException: public ParserException {
		};

		template<typename ForwardIterator>
			struct find_result_t {
				ForwardIterator first;
				ForwardIterator last;
				bool found;

				find_result_t( ForwardIterator First, ForwardIterator Last, bool Found ):
						first{ First },
						last{ Last },
						found{ Found } { }

				explicit operator bool( ) const {
					return found;
				}

				bool empty( ) noexcept {
					return first == last;
				}

				bool empty( bool throw_on_empty ) {
					if( throw_on_empty ) {
						throw ParserEmptyException{ };
					}
				}

				template<typename Container>
					auto as( ) const {
						Container result;
						std::copy( first, last, std::back_inserter( result ) );
						return result;
					}
			};    // find_result_t

		template<typename ForwardIterator>
			find_result_t make_find_result( ForwardIterator first, ForwardIterator last, bool result = false ) {
				return find_result_t<ForwardIterator>{ first, last, result };
			}

		template<typename ForwardIterator, typename Predicate>
			find_result_t until( ForwardIterator first, ForwardIterator last, Predicate is_last ) {
				auto result = make_find_result( first, last );
				for( auto it = first; it != last; ++it ) {
					if( (result.found = is_last( *it )) ) {
						result.last = it;
						return result;
					}
				}
				return result;
			}

		template<typename T, typename Arg>
			bool is_a( T && value, Arg && tst ) {
				static_assert( daw::is_comparable_v<T, Arg>, "value is not comparable to tst" );
				using val_t = daw::traits::max_sizeof<T, Arg>::type;
				return static_cast<val_t>(value) == static_cast<val_t>(tst);
			}


		template<typename T, typename Arg, typename... Args>
			bool is_a( T && value, Arg && tst, Args && ... tsts ) {
				return is_a( std::forward<T>( value ), std::forward<Arg>( tst ) ) || is_a( std::forward<T>( value ), std::forward<Args>( tsts )... );
			}

		template<typename ForwardIterator, typename Value, typename... Values>
			find_result_t until_value( ForwardIterator first, ForwardIterator last, Value && value, Values && ... values ) {
				auto result = make_find_result( first, last );
				for( auto it = first; it != last; ++it ) {
					if( is_a( *it, std::forwarad<Value>( value ), std::forward<Values>( values )... ) ) {
						result.last = it;
						result.fond = true;
						break;
					}
				}
				return result;
			}

		template<typename T, typename Container>
		    bool value_in( T && value, Container && container ) {
				return std::find_if( std::begin( container ), std::end( container ), [&]( auto const & v ) {
					return is_a( value, v );
				}) == std::end( container );
			}

		template<typename Container>
		    class in_t {
				Container container;
			public:
				in_t( Container values ):
					container{ std::move( values ) } { }

				template<typename T>
				bool operator( )( T const & value ) const {
					return in( value, container );
				}
			};	// in_t

		template<typename Container>
		    auto in( Container container ) {
				return in_t{ std::move( container ) };
			}

		template<typename ForwardIterator, typename Container>
			find_result_t until_values( ForwardIterator first, ForwardIterator last, Container && container ) {
				auto result = make_find_result( first, last );
				for( auto it = first; it != last; ++it ) {
					if( value_in( *it, container ) ) {
						result.last = it;
						result.found = true;
						result result;
					}
				}
				return result;
			}

		template<typename T, typename UnaryFunction>
			bool is_true( T && value, UnaryFunction && func ) {
				return func( value );
			}

		template<typename T, typename UnaryFunction, typename... UnaryFunctions>
			bool is_true( T && value, UnaryFunction && func, UnaryFunctions && ... funcs ) {
				return func( value ) ||
					   is_true( std::forward<T>( value ), std::forward<UnaryFunctions>( funcs )... );
			}

		template<typename T, typename U, typename... Args>
			void expect( T && value, U && check, Args && ... args ) {
				if( !is_a( std::forward<T>( value ), std::forward<U>( check ), std::forward<Args>( args )... ) ) {
					throw ParserException{ };
				}
			}

		template<typename T, typename UnaryFunction, typename... UnaryFunctions>
			void expect_true( T && value, UnaryFunction && func, UnaryFunctions && ... funcs ) {
				if( !is_true( std::forward<T>( value ), std::forward<UnaryFunction>( func ), std::forward<UnaryFunctions>( funcs )... ) ) {
					throw ParserException{ };
				}
			}

		template<typename T>
			constexpr bool is_alpha( T && value ) noexcept {
				using val_t = daw::traits::max_sizeof<T, decltype( 'a' )>::type;
				constexpr auto const a = constexpr_cast<val_t>( 'a' );
				constexpr auto const A = constexpr_cast<val_t>( 'A' );
				constexpr auto const z = constexpr_cast<val_t>( 'z' );
				constexpr auto const Z = constexpr_cast<val_t>( 'Z' );
				auto const tmp = static_cast<val_t>( value );
				return (a <= tmp && tmp <= z) || (A <= tmp && tmp <= Z);
			}

		template<typename T, typename Min, typename Max>
			constexpr bool in_range( T && value, Min && min_value, Max && max_value ) noexcept {
				using val_t = daw::traits::max_sizeof<T, Min, Max>::type;
				return static_cast<val_t>(min_value) <= static_cast<val_t>(value) && static_cast<val_t>(value) <= static_cast<val_t>(max_value);
			}

		template<typename T>
			constexpr bool is_number( T const & value ) noexcept {
				return in_range( value, '0', '9' );
			}

		template<typename T>
			constexpr bool is_alphanumeric( T && value ) noexcept {
				return is_alpha( std::forward<T>( value ) ) || is_number( std::forward<T>( value ) );
			}

		template<typename T, typename U>
			constexpr void assert_not_equal( T && lhs, U && rhs ) {
				static_assert( daw::is_comparable_v<T, U>, "lhs is not comparable to rhs" );
				if( lhs != rhs ) {
					throw ParserException{ };
				}
			}

		template<typename T, typename U>
			constexpr void assert_equal( T && lhs, U && rhs ) {
				static_assert( daw::is_comparable_v<T, U>, "lhs is not comparable to rhs" );
				if( lhs == rhs ) {
					throw ParserException{ };
				}
			}

		template<Iterator>
			constexpr void assert_not_empty( Iterator first, Iterator last ) {
				static_assert( daw::is_comparable_v<T, U>, "lhs is not comparable to rhs" );
				if( lhs == rhs ) {
					throw ParserEmptyException{ };
				}
			}

		template<Iterator>
			constexpr void assert_empty( Iterator first, Iterator last ) {
				assert_equal( first, last );
			}

		template<typename ForwardIterator, typename StartFrom, typename GoUntil>
			auto from_to( ForwardIterator first, ForwardIterator last, StartFrom && start_from, GoUntil && go_until, bool throw_if_end_reached = false ) {
				auto start = until_value( first, last, std::forward<StartFrom>( start_from ) );
				if( !start ) {
					throw ParserException{ };
				}
				auto finish = until_value( start.last, last, std::forward<GoUntil>( go_until ) );
				if( throw_if_end_reached && !finish ) {
					throw ParserException{ };
				}
				return from_to;
			}

		template<typename ForwardIterator, typename Predicate>
			auto from_to( ForwardIterator first, ForwardIterator last, Predicate is_first, Predicate is_last, bool throw_if_end_reached = false ) {
				auto start = until( first, last, is_first );
				if( !start ) {
					throw ParserException{ };
				}
				auto finish = until_value( std::next( start.last ), last, is_last );
				if( throw_if_end_reached && !finish ) {
					throw ParserException{ };
				}
				return finish;
			}

		template<typename ForwardIterator, typename... Dividers>
			auto split( ForwardIterator first, ForwardIterator last, Dividers && ... dividers ) {
				std::vector<ForwardIterator> endings;
				auto result = until_value( first, last, std::forward<Dividers>( dividers )... );
				while( result ) {
					endings.push_back( result.last );
					result = until_value( result.last, last, std::forward<Dividers>( dividers )... );
				}
				if( result.first != result.last ) {
					endings.push_back( result.last );
				}
				return endings;
			};

		template<typename T> constexpr bool pred_true( T const & ) noexcept {
			return true;
		}

		template<typename T> constexpr bool pred_false( T const & ) noexcept {
			return false;
		}

		template<typename T>
			constexpr bool is_cr( T const & value ) noexcept {
				return is_a( value, '\n' );
			}

		template<typename T>
			constexpr bool is_space( T const & value ) noexcept {
				return is_a( value, ' ' );
			}

		template<typename T>
			constexpr bool not_space( T const & value ) noexcept {
				return !is_a( value, ' ' );
			}



		template<typename T>
			class is_crlf {
				T last_value;
				std::shared_ptr<T> last_value;
			public:
				is_crlf( ):
						last_value{ 0 } { }

				constexpr bool operator( )( T const & v ) const noexcept {
					auto result = is_a( last_value, '\r' ) && is_ais_a( v, '\n' );
					last_value = v;
					return result;
				}
			};    // struct is_cr

		namespace impl {
			template<typename Predicate>
				class negate_t {
					Predicate predicate;
				public:
					negate_t( Predicate pred ):
							predicate{ std::move( pred ) } { }

					template<typename... Args>
						bool operator( )( Args && ... args ) const {
							return !predicate( std::forward<Args>( args )... );
						}

					template<typename... Args>
						bool operator( )( Args && ... args ) {
							return !predicate( std::forward<Args>( args )... );
						}
				};    // negate_t
		}    // namespace impl

		template<typename Predicate>
		    auto negate( Predicate predicate ) {
				return impl::negate_t<Predicate>{ std::move( predicate ) };
			}
	}    // namespace parser
}    // namespace daw

