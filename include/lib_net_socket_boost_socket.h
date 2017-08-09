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
				namespace impl {

					struct BoostSocket {
						using EncryptionContext = boost::asio::ssl::context;
						using BoostSocketValueType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

					  private:
						std::shared_ptr<EncryptionContext> m_encryption_context;
						bool m_encryption_enabled;
						std::shared_ptr<BoostSocketValueType> m_socket;


					  public:
						BoostSocketValueType &raw_socket( );

						BoostSocketValueType const &raw_socket( ) const;
						BoostSocket( );
						BoostSocket( std::shared_ptr<EncryptionContext> context );
						BoostSocket( std::shared_ptr<BoostSocketValueType> socket,
						             std::shared_ptr<EncryptionContext> context );

						~BoostSocket( );
						BoostSocket( BoostSocket const & ) = default;
						BoostSocket &operator=( BoostSocket const & ) = default;
						BoostSocket( BoostSocket && ) = default;
						BoostSocket &operator=( BoostSocket && ) = default;

						friend void swap( BoostSocket &lhs, BoostSocket &rhs ) noexcept;

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
						std::shared_ptr<EncryptionContext> const & context( ) const;

						void reset_socket( );

						EncryptionContext &encryption_context( );

						EncryptionContext const &encryption_context( ) const;

						bool is_open( ) const;

						void shutdown( );

						boost::system::error_code shutdown( boost::system::error_code &ec ) noexcept;

						void close( );

						boost::system::error_code close( boost::system::error_code &ec );

						void cancel( );

						boost::asio::ip::tcp::endpoint remote_endpoint( ) const;

						boost::asio::ip::tcp::endpoint local_endpoint( ) const;

						template<typename HandshakeHandler>
						void async_handshake( BoostSocketValueType::handshake_type role, HandshakeHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							m_socket->async_handshake( role, handler );
						}

						template<typename ShutdownHandler>
						void async_shutdown( ShutdownHandler handler ) {
							m_socket->async_shutdown( handler );
						}

						template<typename ConstBufferSequence, typename WriteHandler>
						void async_write( ConstBufferSequence const &buffer, WriteHandler handler ) {
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
						void async_read( MutableBufferSequence &buffer, ReadHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							if( encryption_on( ) ) {
								boost::asio::async_read( *m_socket, buffer, handler );
							} else {
								boost::asio::async_read( m_socket->next_layer( ), buffer, handler );
							}
						}

						template<typename MutableBufferSequence, typename MatchType, typename ReadHandler>
						void async_read_until( MutableBufferSequence &buffer, MatchType &&m, ReadHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							if( encryption_on( ) ) {
								boost::asio::async_read_until( *m_socket, buffer, std::forward<MatchType>( m ),
								                               handler );
							} else {
								boost::asio::async_read_until( m_socket->next_layer( ), buffer,
								                               std::forward<MatchType>( m ), handler );
							}
						}

						template<typename Iterator, typename ComposedConnectHandler>
						void async_connect( Iterator it, ComposedConnectHandler handler ) {
							init( );
							daw::exception::daw_throw_on_false( m_socket, "Invalid socket" );
							boost::asio::async_connect( m_socket->next_layer( ), it, handler );
						}

						void enable_encryption( boost::asio::ssl::stream_base::handshake_type handshake );
					};

					// BoostSocket create_boost_socket( boost::asio::io_service & io_service );
					// BoostSocket create_boost_socket( boost::asio::io_service & io_service,
					// std::shared_ptr<BoostSocket::EncryptionContext> context );
				} // namespace impl
			}     // namespace net
		}         // namespace lib
	}             // namespace nodepp
} // namespace daw
  // TOOD remove #undef create_visitor
