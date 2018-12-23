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

#pragma once

#include <asio/read_until.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <type_traits>

#include <daw/daw_exception.h>
#include <daw/daw_memory_mapped_file.h>

#include "base_error.h"
#include "base_types.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using EncryptionContext = asio::ssl::context;

				enum class ip_version : uint_fast8_t { ipv4, ipv6, ipv4_v6 };

				struct SslServerConfig
				  : public daw::json::daw_json_link<SslServerConfig> {
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

				namespace nss_impl {
					struct BoostSocket {
						using BoostSocketValueType =
						  asio::ssl::stream<asio::ip::tcp::socket>;

					private:
						std::unique_ptr<EncryptionContext> m_encryption_context{};
						std::unique_ptr<BoostSocketValueType> m_socket{};
						bool m_encryption_enabled = false;

						BoostSocketValueType &raw_socket( );
						BoostSocketValueType const &raw_socket( ) const;

					public:
						constexpr BoostSocket( ) noexcept = default;

						explicit BoostSocket( std::unique_ptr<EncryptionContext> context );
						explicit BoostSocket( SslServerConfig const &ssl_config );

						BoostSocket( std::unique_ptr<BoostSocketValueType> &&socket,
						             std::unique_ptr<EncryptionContext> &&context );

						explicit operator bool( ) const;

						void init( );

						BoostSocketValueType const &operator*( ) const;

						BoostSocketValueType &operator*( );
						BoostSocketValueType *operator->( ) const;
						BoostSocketValueType *operator->( );

						bool encyption_on( ) const;
						bool &encryption_on( );
						void encyption_on( bool value );

						EncryptionContext &context( );
						EncryptionContext const &context( ) const;

						EncryptionContext &encryption_context( );
						EncryptionContext const &encryption_context( ) const;

						void ip6_only( bool value );
						bool ip6_only( ) const;

						void reset_socket( );
						bool is_open( ) const;

						void shutdown( );
						std::error_code shutdown( std::error_code &ec ) noexcept;

						void close( );
						std::error_code close( std::error_code &ec );

						void cancel( );

						asio::ip::tcp::endpoint remote_endpoint( ) const;
						asio::ip::tcp::endpoint local_endpoint( ) const;

						template<typename HandshakeHandler>
						void handshake_async( BoostSocketValueType::handshake_type role,
						                      HandshakeHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							m_socket->async_handshake( role, handler );
						}

						template<typename ShutdownHandler>
						void shutdown_shutdown( ShutdownHandler handler ) {
							m_socket->async_shutdown( handler );
						}

						template<typename ConstBufferSequence, typename WriteHandler>
						void write_async( ConstBufferSequence &&buffer,
						                  WriteHandler&& handler ) {
							init( );
							daw::exception::precondition_check( m_socket, "Invalid socket" );
							daw::exception::precondition_check(
							  is_open( ), "Attempt to write to closed socket" );
							if( encryption_on( ) ) {
								asio::async_write( *m_socket, buffer, std::forward<WriteHandler>( handler ) );
							} else {
								asio::async_write( m_socket->next_layer( ), buffer, std::forward<WriteHandler>( handler ) );
							}
						}

						template<typename ConstBufferSequence>
						void write( ConstBufferSequence const &buffer ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							daw::exception::daw_throw_on_false(
							  is_open( ), "Attempt to write to closed socket" );
							if( encryption_on( ) ) {
								asio::write( *m_socket, buffer );
							} else {
								asio::write( m_socket->next_layer( ), buffer );
							}
						}

						void write_file( daw::string_view file_name );

						template<typename MutableBufferSequence, typename ReadHandler>
						void read_async( MutableBufferSequence &buffer,
						                 ReadHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							if( encryption_on( ) ) {
								asio::async_read( *m_socket, buffer, handler );
							} else {
								asio::async_read( m_socket->next_layer( ), buffer, handler );
							}
						}

						template<typename MutableBufferSequence, typename MatchType,
						         typename ReadHandler>
						void read_until_async( MutableBufferSequence &buffer, MatchType &&m,
						                       ReadHandler handler ) {
							init( );
							daw::exception::precondition_check( m_socket, "Invalid socket" );

							if( encryption_on( ) ) {
								asio::async_read_until( *m_socket, buffer,
								                        std::forward<MatchType>( m ), handler );
							} else {
								asio::async_read_until( m_socket->next_layer( ), buffer,
								                        std::forward<MatchType>( m ), handler );
							}
						}

						template<typename Iterator, typename ComposedConnectHandler>
						void connect_async( Iterator &&it,
						                    ComposedConnectHandler &&handler ) {

							static_assert(
							  std::is_invocable_v<ComposedConnectHandler,
							                      daw::nodepp::base::ErrorCode,
							                      asio::ip::tcp::resolver::iterator>,
							  "Connection handler must accept an error_code and "
							  "and endpoint as arguments" );
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );

							asio::async_connect(
							  m_socket->next_layer( ), std::forward<Iterator>( it ),
							  std::forward<ComposedConnectHandler>( handler ) );
						}

						void enable_encryption(
						  asio::ssl::stream_base::handshake_type handshake );
					};
				} // namespace nss_impl
			}   // namespace net
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
