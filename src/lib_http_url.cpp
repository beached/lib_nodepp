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

#include "lib_http_url.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				UrlAuthInfo::UrlAuthInfo( daw::string_view UserName, daw::string_view Password )
				    : username{UserName.to_string( )}, password{Password.to_string( )} {}

				std::string to_string( UrlAuthInfo const &auth ) {
					std::stringstream ss;
					ss << auth.username << ":" << auth.password;
					return ss.str( );
				}

				std::ostream &operator<<( std::ostream &os, UrlAuthInfo const &auth ) {
					os << to_string( auth );
					return os;
				}

				HttpUrlQueryPair::HttpUrlQueryPair( std::pair<std::string, boost::optional<std::string>> const &vals )
				    : name{vals.first}, value{vals.second} {}

				void HttpUrlQueryPair::json_link_map( ) {
					link_json_string( "name", name );
					link_json_string_optional( "value", value, boost::none );
				}

				void HttpAbsoluteUrlPath::json_link_map( ) {
					link_json_string( "path", path );
					link_json_object_array( "query", query );
					link_json_string_optional( "fragment", fragment, boost::none );
				}

				bool HttpAbsoluteUrlPath::query_exists( daw::string_view name ) const noexcept {
					return std::find_if( query.cbegin( ), query.cend( ), [name]( auto const & qp ) {
						return qp.name == name;
					}) == query.cend( );
				}

				boost::optional<std::string> HttpAbsoluteUrlPath::query_get( daw::string_view name ) const {
					auto it = std::find_if( query.cbegin( ), query.cend( ), [name]( auto const & qp ) {
						return qp.name == name;
					});
					if( it == query.cend( ) ) {
						return boost::none;
					}
					return it->value;
				}

				std::string to_string( HttpAbsoluteUrlPath const &url_path ) {
					std::stringstream ss;
					ss << url_path.path;
					if( !url_path.query.empty( ) ) {
						for( auto const &qp : url_path.query ) {
							ss << "?" << qp.name;
							if( qp.value ) {
								ss << "=" << qp.value.get( );
							}
						}
					}
					if( url_path.fragment ) {
						ss << "#" << url_path.fragment.get( );
					}
					return ss.str( );
				}

				std::ostream &operator<<( std::ostream &os, HttpAbsoluteUrlPath const &url_path ) {
					os << to_string( url_path );
					return os;
				}

				void UrlAuthInfo::json_link_map( ) {
					link_json_string( "username", username );
					link_json_string( "password", password );
				}

				namespace impl {
					void HttpUrlImpl::json_link_map( ) {
						link_json_string( "scheme", scheme );
						link_json_object_optional( "auth_info", auth_info, boost::none );
						link_json_string( "host", host );
						link_json_integer_optional( "port", port, boost::none );
						link_json_object_optional( "path", path, boost::none );
					}
				} // namespace impl

				std::string to_string( HttpUrl const &url ) {
					return url ? to_string( *url ) : "";
				}

				std::string to_string( impl::HttpUrlImpl const &url ) {
					std::stringstream ss;
					ss << url.scheme << "://";
					ss << url.host;
					if( url.port ) {
						ss << *( url.port );
					}

					if( url.auth_info ) {
						ss << *( url.auth_info ) << "@";
					}

					if( url.path ) {
						ss << "/" << *( url.path );
					}

					return ss.str( );
				}

				std::ostream &operator<<( std::ostream &os, http::HttpUrl const &url ) {
					if( url ) {
						os << to_string( *url );
					}
					return os;
				}

				std::ostream &operator<<( std::ostream &os, impl::HttpUrlImpl const &url ) {
					os << to_string( url );
					return os;
				}

			} // namespace http
		}     // namespace lib
	}         // namespace nodepp
} // namespace daw
