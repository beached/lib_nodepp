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

#include <asio.hpp>
#include <boost/filesystem.hpp>

#include <daw/daw_exception.h>
#include <daw/daw_utility.h>
#include <daw/json/daw_json_link.h>

#include "base_service_handle.h"
#include "lib_net_socket_asio_socket.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				std::string SslServerConfig::get_tls_ca_verify_file( ) const {
					if( tls_ca_verify_file.empty( ) ) {
						return tls_ca_verify_file;
					}
					boost::filesystem::path p{tls_ca_verify_file};
					return canonical( p ).string( );
				}

				std::string SslServerConfig::get_tls_certificate_chain_file( ) const {
					if( tls_certificate_chain_file.empty( ) ) {
						return tls_certificate_chain_file;
					}
					boost::filesystem::path p{tls_certificate_chain_file};
					return canonical( p ).string( );
				}

				std::string SslServerConfig::get_tls_private_key_file( ) const {
					if( tls_private_key_file.empty( ) ) {
						return tls_private_key_file;
					}
					boost::filesystem::path p{tls_private_key_file};
					return canonical( p ).string( );
				}

				std::string SslServerConfig::get_tls_dh_file( ) const {
					if( tls_dh_file.empty( ) ) {
						return tls_dh_file;
					}
					boost::filesystem::path p{tls_dh_file};
					return canonical( p ).string( );
				}

				namespace nss_impl {
					BoostSocket::BoostSocket( std::unique_ptr<EncryptionContext> context )
					  : m_encryption_context( daw::move( context ) )
					  , m_encryption_enabled(
					      static_cast<bool>( m_encryption_context ) ) {}

					BoostSocket::BoostSocket(
					  std::unique_ptr<BoostSocket::BoostSocketValueType> &&socket,
					  std::unique_ptr<EncryptionContext> &&context )
					  : m_encryption_context( daw::move( context ) )
					  , m_socket( daw::move( socket ) )
					  , m_encryption_enabled(
					      static_cast<bool>( m_encryption_context ) ) {}

					namespace {
						std::unique_ptr<EncryptionContext>
						make_context( SslServerConfig const &ssl_config ) {
							auto context = std::make_unique<EncryptionContext>(
							  EncryptionContext::tlsv12_server );

							context->set_options( EncryptionContext::default_workarounds |
							                      EncryptionContext::no_sslv2 |
							                      EncryptionContext::no_sslv3 |
							                      EncryptionContext::single_dh_use );

							if( !ssl_config.tls_certificate_chain_file.empty( ) ) {
								context->use_certificate_chain_file(
								  ssl_config.get_tls_certificate_chain_file( ) );
							}

							if( !ssl_config.tls_private_key_file.empty( ) ) {
								context->use_private_key_file(
								  ssl_config.get_tls_private_key_file( ),
								  EncryptionContext::file_format::pem );
							}

							if( !ssl_config.tls_dh_file.empty( ) ) {
								context->use_tmp_dh_file( ssl_config.get_tls_dh_file( ) );
							}
							return context;
						}
					} // namespace

					BoostSocket::BoostSocket( SslServerConfig const &ssl_config )
					  : BoostSocket( make_context( ssl_config ) ) {}

					void BoostSocket::init( bool must_exist ) {
						if( !m_encryption_context ) {
							m_encryption_context = std::make_unique<EncryptionContext>(
							  EncryptionContext::tlsv12 );
						}
						if( !m_socket ) {
							m_socket = std::make_unique<BoostSocketValueType>(
							  base::ServiceHandle::get( ), *m_encryption_context );
						}
						daw::exception::precondition_check( !must_exist or
						  m_socket, "Could not create asio socket" );
					}

					void BoostSocket::reset_socket( ) {
						m_socket.reset( );
					}

					EncryptionContext &BoostSocket::encryption_context( ) {
						daw::exception::precondition_check(
						  m_encryption_context,
						  "Attempt to retrieve an invalid encryption context" );
						return *m_encryption_context;
					}

					EncryptionContext const &BoostSocket::encryption_context( ) const {
						daw::exception::precondition_check(
						  m_encryption_context,
						  "Attempt to retrieve an invalid encryption context" );
						return *m_encryption_context;
					}

					BoostSocket::BoostSocketValueType &BoostSocket::raw_socket( ) {
						init( );
						return *m_socket;
					}

					BoostSocket::BoostSocketValueType const &
					BoostSocket::raw_socket( ) const {
						daw::exception::precondition_check( m_socket, "Invalid socket" );
						return *m_socket;
					}

					BoostSocket::operator bool( ) const {
						return static_cast<bool>( m_socket );
					}

					BoostSocket::BoostSocketValueType const &BoostSocket::
					operator*( ) const {
						return raw_socket( );
					}

					BoostSocket::BoostSocketValueType &BoostSocket::operator*( ) {
						return raw_socket( );
					}

					BoostSocket::BoostSocketValueType *BoostSocket::operator->( ) const {
						daw::exception::precondition_check( m_socket,
						                                    "Invalid socket - null" );
						return m_socket.operator->( );
					}

					BoostSocket::BoostSocketValueType *BoostSocket::operator->( ) {
						init( );
						return m_socket.operator->( );
					}

					bool BoostSocket::encyption_on( ) const {
						return m_encryption_enabled;
					}

					bool &BoostSocket::encryption_on( ) {
						return m_encryption_enabled;
					}

					EncryptionContext &BoostSocket::context( ) {
						return *m_encryption_context;
					}

					EncryptionContext const &BoostSocket::context( ) const {
						return *m_encryption_context;
					}

					void BoostSocket::encyption_on( bool value ) {
						m_encryption_enabled = value;
					}

					bool BoostSocket::is_open( ) {
						init( false );
						if( !m_socket ) {
							return false;
						}
						return raw_socket( ).next_layer( ).is_open( );
					}

					bool BoostSocket::is_open( ) const {
						if( !m_socket ) {
							return false;
						}
						return raw_socket( ).next_layer( ).is_open( );
					}

					void BoostSocket::shutdown( ) {
						if( encryption_on( ) ) {
							// raw_socket( ).shutdown( );
						}
						raw_socket( ).lowest_layer( ).shutdown(
						  asio::socket_base::shutdown_both );
					}

					std::error_code
					BoostSocket::shutdown( std::error_code &ec ) noexcept {
						if( encryption_on( ) ) {
							raw_socket( ).shutdown( );
							ec = raw_socket( ).shutdown( ec );
							if( static_cast<bool>( ec ) ) {
								return ec;
							}
						}
						return raw_socket( ).lowest_layer( ).shutdown(
						  asio::socket_base::shutdown_both, ec );
					}

					void BoostSocket::close( ) {
						raw_socket( ).shutdown( );
					}

					std::error_code BoostSocket::close( std::error_code &ec ) {
						return raw_socket( ).shutdown( ec );
					}

					void BoostSocket::cancel( ) {
						raw_socket( ).next_layer( ).cancel( );
					}

					asio::ip::tcp::endpoint BoostSocket::remote_endpoint( ) const {
						return raw_socket( ).next_layer( ).remote_endpoint( );
					}

					asio::ip::tcp::endpoint BoostSocket::local_endpoint( ) const {
						return raw_socket( ).next_layer( ).local_endpoint( );
					}

					void BoostSocket::ip6_only( bool value ) {
						asio::ip::v6_only option{value};
						raw_socket( ).next_layer( ).set_option( option );
					}

					bool BoostSocket::ip6_only( ) const {
						asio::ip::v6_only option;
						m_socket->lowest_layer( ).get_option( option );
						return option.value( );
					}

				} // namespace nss_impl
			}   // namespace net
		}     // namespace lib
	}       // namespace nodepp
} // namespace daw
