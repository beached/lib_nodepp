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

#include <boost/asio.hpp>

#include <daw/daw_exception.h>
#include <daw/daw_memory_mapped_file.h>

#include "base_types.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using EncryptionContext = boost::asio::ssl::context;

				enum class ip_version : uint_fast8_t { ipv4, ipv6, ipv4_v6 };

				struct SslServerConfig : public daw::json::daw_json_link<SslServerConfig> {
					std::string tls_ca_verify_file;
					std::string tls_certificate_chain_file;
					std::string tls_private_key_file;
					std::string tls_dh_file;

					static void json_link_map( );

					std::string get_tls_ca_verify_file( ) const;
					std::string get_tls_certificate_chain_file( ) const;
					std::string get_tls_private_key_file( ) const;
					std::string get_tls_dh_file( ) const;
				};

				namespace impl {
					struct BoostSocket {
						using BoostSocketValueType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

					private:
						std::shared_ptr<EncryptionContext> m_encryption_context;
						std::shared_ptr<BoostSocketValueType> m_socket;
						bool m_encryption_enabled;

						BoostSocketValueType &raw_socket( );
						BoostSocketValueType const &raw_socket( ) const;

					public:
						constexpr BoostSocket( ) noexcept
						  : m_encryption_context{nullptr}, m_socket{nullptr}, m_encryption_enabled{false} {}

						explicit BoostSocket( std::shared_ptr<EncryptionContext> context );
						explicit BoostSocket( SslServerConfig const &ssl_config );

						BoostSocket( std::shared_ptr<BoostSocketValueType> socket, std::shared_ptr<EncryptionContext> context );

						~BoostSocket( ) = default;
						BoostSocket( BoostSocket const & ) = default;
						BoostSocket &operator=( BoostSocket const & ) = default;
						BoostSocket( BoostSocket && ) noexcept = default;
						BoostSocket &operator=( BoostSocket && ) noexcept = default;

						explicit operator bool( ) const;

						void init( );

						BoostSocketValueType const &operator*( ) const;

						BoostSocketValueType &operator*( );
						BoostSocketValueType *operator->( ) const;
						BoostSocketValueType *operator->( );

						bool encyption_on( ) const;
						bool &encryption_on( );
						void encyption_on( bool value );

						std::shared_ptr<EncryptionContext> context( );
						std::shared_ptr<EncryptionContext> const &context( ) const;

						EncryptionContext &encryption_context( );
						EncryptionContext const &encryption_context( ) const;

						void ip6_only( bool value );
						bool ip6_only( ) const;

						void reset_socket( );
						bool is_open( ) const;

						void shutdown( );
						boost::system::error_code shutdown( boost::system::error_code &ec ) noexcept;

						void close( );
						boost::system::error_code close( boost::system::error_code &ec );

						void cancel( );

						boost::asio::ip::tcp::endpoint remote_endpoint( ) const;
						boost::asio::ip::tcp::endpoint local_endpoint( ) const;

						template<typename HandshakeHandler>
						void handshake_async( BoostSocketValueType::handshake_type role, HandshakeHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							m_socket->async_handshake( role, handler );
						}

						template<typename ShutdownHandler>
						void shutdown_shutdown( ShutdownHandler handler ) {
							m_socket->async_shutdown( handler );
						}

						template<typename ConstBufferSequence, typename WriteHandler>
						void write_async( ConstBufferSequence const & buffer, WriteHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							daw::exception::daw_throw_on_false( is_open( ), "Attempt to write to closed socket" );
							if( encryption_on( ) ) {
								boost::asio::async_write( *m_socket, buffer, handler );
							} else {
								boost::asio::async_write( m_socket->next_layer( ), buffer, handler );
							}
						}

						template<typename ConstBufferSequence>
						void write( ConstBufferSequence const &buffer ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							daw::exception::daw_throw_on_false( is_open( ), "Attempt to write to closed socket" );
							if( encryption_on( ) ) {
								boost::asio::write( *m_socket, buffer );
							} else {
								boost::asio::write( m_socket->next_layer( ), buffer );
							}
						}

						void write_file( daw::string_view file_name );

						template<typename MutableBufferSequence, typename ReadHandler>
						void read_async( MutableBufferSequence &buffer, ReadHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							if( encryption_on( ) ) {
								boost::asio::async_read( *m_socket, buffer, handler );
							} else {
								boost::asio::async_read( m_socket->next_layer( ), buffer, handler );
							}
						}

						template<typename MutableBufferSequence, typename MatchType, typename ReadHandler>
						void read_until_async( MutableBufferSequence &buffer, MatchType &&m, ReadHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							if( encryption_on( ) ) {
								boost::asio::async_read_until( *m_socket, buffer, std::forward<MatchType>( m ), handler );
							} else {
								boost::asio::async_read_until( m_socket->next_layer( ), buffer, std::forward<MatchType>( m ), handler );
							}
						}

						template<typename Iterator, typename ComposedConnectHandler>
						void connect_async( Iterator it, ComposedConnectHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							boost::asio::async_connect( m_socket->next_layer( ), it, handler );
						}

						void enable_encryption( boost::asio::ssl::stream_base::handshake_type handshake );
					};
				} // namespace impl
			}   // namespace net
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
  // TOOD remove #undef create_visitor
