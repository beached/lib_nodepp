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

#include <boost/optional.hpp>
#include <map>

#include "base_types.h"

namespace daw {
	namespace nodepp {
		namespace base {
			struct Url {
				using query_t = std::map<std::string, std::string>;
				boost::optional<std::string> root;
				boost::optional<std::string> hierarchy;
				boost::optional<query_t> queries;
			};

			std::shared_ptr<Url> parse_url( daw::nodepp::base::data_t::iterator first,
			                                daw::nodepp::base::data_t::iterator last );

			enum class uri_parts : size_t { Scheme = 0, User, Password, Host, Port, Path, Query, Fragment };

			namespace uri_detectors {
				template<typename UriT>
				using has_scheme_view = decltype( std::declval<UriT>( ).scheme( ) );

				template<typename UriT>
				using has_user_view = decltype( std::declval<UriT>( ).user( ) );

				template<typename UriT>
				using has_password_view = decltype( std::declval<UriT>( ).password( ) );

				template<typename UriT>
				using has_host_view = decltype( std::declval<UriT>( ).host( ) );

				template<typename UriT>
				using has_port_view = decltype( std::declval<UriT>( ).port( ) );

				template<typename UriT>
				using has_path_view = decltype( std::declval<UriT>( ).path( ) );

				template<typename UriT>
				using has_query_view = decltype( std::declval<UriT>( ).query( ) );

				template<typename UriT>
				using has_fragment_view = decltype( std::declval<UriT>( ).fragment( ) );

				template<typename UriT>
				using has_uri_view = decltype( std::declval<UriT>( ).uri( ) );
			} // namespace uri_detectors

			template<typename UriT>
			constexpr bool is_uri_view_v =
			  daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_scheme_view, UriT>
			    &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_user_view, UriT>
			      &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_password_view, UriT>
			        &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_host_view, UriT>
			          &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_port_view, UriT>
			            &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_path_view, UriT>
			              &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_query_view, UriT>
			                &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_fragment_view, UriT>
			                  &&daw::is_detected_convertible_v<daw::string_view, uri_detectors::has_uri_view, UriT>;

			template<typename UriParser, typename SizeT = uint16_t>
			struct basic_uri_view {
				using pointer = char const *;
				using size_type = SizeT;

			private:
				pointer m_first;
				daw::static_array_t<size_type, 8> m_part_lengths;

			public:
				~basic_uri_view( ) noexcept = default;
				constexpr basic_uri_view( basic_uri_view const & ) noexcept = default;
				constexpr basic_uri_view( basic_uri_view && ) noexcept = default;
				constexpr basic_uri_view &operator=( basic_uri_view const & ) noexcept = default;
				constexpr basic_uri_view &operator=( basic_uri_view && ) noexcept = default;

				constexpr basic_uri_view( ) noexcept : m_first{nullptr}, m_part_lengths{0} {}

				constexpr basic_uri_view( daw::string_view uri_str ) noexcept(
				  noexcept( UriParser{}( uri_parts::Scheme, std::declval<daw::string_view>( ), 0 ) ) )
				  : m_first{uri_str.data( )}, m_part_lengths{0} {

					auto const parser = UriParser{};

					auto pos = parser( uri_parts::Scheme, uri_str, 0 );
					m_part_lengths[static_cast<size_t>( uri_parts::Scheme )] = pos;

					pos = parser( uri_parts::User, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::User )] = pos;

					pos = parser( uri_parts::Password, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::Password )] = pos;

					pos = parser( uri_parts::Host, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::Host )] = pos;

					pos = parser( uri_parts::Port, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::Port )] = pos;

					pos = parser( uri_parts::Path, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::Path )] = pos;

					pos = parser( uri_parts::Query, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::Query )] = pos;

					pos = parser( uri_parts::Fragment, uri_str, pos );
					m_part_lengths[static_cast<size_t>( uri_parts::Fragment )] = pos;
				}

			private:
				template<uri_parts part>
				constexpr size_type get_start( ) const noexcept {
					size_t start = 0;
					for( size_t n = 0; n < static_cast<size_t>( part ); ++n ) {
						start += m_part_lengths[n];
					}
					return start;
				}

				template<uri_parts part>
				constexpr daw::string_view get_part( ) const noexcept {
					size_t const start = get_start<part>( );
					return daw::string_view{m_first + start, m_part_lengths[static_cast<size_t>( part )]};
				}

			public:
				constexpr daw::string_view scheme( ) const noexcept {
					return get_part<uri_parts::Scheme>( );
				}

				constexpr daw::string_view user( ) const noexcept {
					return get_part<uri_parts::User>( );
				}

				constexpr daw::string_view password( ) const noexcept {
					return get_part<uri_parts::Password>( );
				}

				constexpr daw::string_view host( ) const noexcept {
					return get_part<uri_parts::Host>( );
				}

				constexpr daw::string_view port( ) const noexcept {
					return get_part<uri_parts::Port>( );
				}

				constexpr daw::string_view path( ) const noexcept {
					return get_part<uri_parts::Path>( );
				}

				constexpr daw::string_view query( ) const noexcept {
					return get_part<uri_parts::Query>( );
				}

				constexpr daw::string_view fragment( ios::binary ) const noexcept {
					return get_part<uri_parts::Fragment>( );
				}

				constexpr daw::string_view uri( ) const noexcept {
					return daw::string_view{m_first,
					                        get_start<uri_parts::Fragment>( ) + static_cast<size_t>( uri_parts::Fragment )};
				}
			};

			template<typename UriParser, typename SizeT = uint16_t>
			class basic_uri_buffer {
				std::string m_buffer;
				basic_uri_view<UriParser, SizeT> m_view;

			public:
				basic_uri_buffer( ) noexcept : m_buffer{}, m_view{} {}
				basic_uri_buffer( std::string uri_str ) noexcept : m_buffer{std::move( uri_str )}, m_view{m_buffer} {}

				std::string const &str( ) const {
					return m_buffer;
				}

				void str( std::string uri_str ) noexcept {
					m_buffer = std::move( uri_str );
					m_view = basic_uri_view<UriParser, SizeT>{m_buffer};
				}

				constexpr daw::string_view scheme( ) const noexcept {
					return m_view.scheme( );
				}

				constexpr daw::string_view user( ) const noexcept {
					return m_view.user( );
				}

				constexpr daw::string_view password( ) const noexcept {
					return m_view.password( );
				}

				constexpr daw::string_view host( ) const noexcept {
					return m_view.host( );
				}

				constexpr daw::string_view port( ) const noexcept {
					return m_view.port( );
				}

				constexpr daw::string_view path( ) const noexcept {
					return m_view.path( );
				}

				constexpr daw::string_view query( ) const noexcept {
					return m_view.query( );
				}

				constexpr daw::string_view fragment( ) const noexcept {
					return m_view.fragment( );
				}

				constexpr daw::string_view uri( ) const noexcept {
					return m_view.uri( );
				}
			};
		} // namespace base
	}   // namespace nodepp
} // namespace daw

