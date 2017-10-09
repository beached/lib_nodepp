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
#include <boost/variant.hpp>
#include <cstdint>
#include <memory>
#include <string>

#include <daw/daw_string_view.h>

#include "base_enoding.h"
#include "base_error.h"
#include "base_selfdestruct.h"
#include "base_semaphore.h"
#include "base_service_handle.h"
#include "base_stream.h"
#include "base_types.h"
#include "base_write_buffer.h"
#include "lib_net_dns.h"
#include "lib_net_socket_boost_socket.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using EndPoint = boost::asio::ip::tcp::endpoint;
				namespace impl {
					struct NetSocketStreamImpl;
				}

				using NetSocketStream = std::shared_ptr<impl::NetSocketStreamImpl>;

				enum class NetSocketStreamReadMode : uint_fast8_t {
					newline,
					buffer_full,
					predicate,
					next_byte,
					regex,
					values,
					double_newline
				};

				NetSocketStream create_net_socket_stream( base::EventEmitter emitter = base::create_event_emitter( ) );

				NetSocketStream create_net_socket_stream( SslServerConfig const &ssl_config,
				                                          base::EventEmitter emitter = base::create_event_emitter( ) );

				namespace impl {
					struct NetSocketStreamImpl : public daw::nodepp::base::SelfDestructing<NetSocketStreamImpl>,
					                             public daw::nodepp::base::stream::StreamReadableEvents<NetSocketStreamImpl>,
					                             public daw::nodepp::base::stream::StreamWritableEvents<NetSocketStreamImpl> {

						using match_iterator_t = boost::asio::buffers_iterator<base::stream::StreamBuf::const_buffers_type>;
						using match_function_t =
						  std::function<std::pair<match_iterator_t, bool>( match_iterator_t begin, match_iterator_t end )>;

					private:
						struct netsockstream_state_t {
							bool closed;
							bool end;

							constexpr netsockstream_state_t( ) noexcept : closed{false}, end{false} {}
							~netsockstream_state_t( ) = default;
							constexpr netsockstream_state_t( netsockstream_state_t const & ) noexcept = default;
							constexpr netsockstream_state_t( netsockstream_state_t && ) noexcept = default;
							constexpr netsockstream_state_t &operator=( netsockstream_state_t const & ) noexcept = default;
							constexpr netsockstream_state_t &operator=( netsockstream_state_t && ) noexcept = default;
						};

						struct netsockstream_readoptions_t {
							size_t max_read_size = 8192;
							std::unique_ptr<NetSocketStreamImpl::match_function_t> read_predicate;
							std::string read_until_values;
							NetSocketStreamReadMode read_mode = NetSocketStreamReadMode::newline;

							netsockstream_readoptions_t( ) = default;
							~netsockstream_readoptions_t( ) = default;

							explicit netsockstream_readoptions_t( size_t max_read_size_ )
							  : max_read_size{max_read_size_}, read_mode{NetSocketStreamReadMode::newline} {}

							netsockstream_readoptions_t( netsockstream_readoptions_t const & ) = delete;
							netsockstream_readoptions_t( netsockstream_readoptions_t && ) noexcept = default;
							netsockstream_readoptions_t &operator=( netsockstream_readoptions_t const & ) = delete;
							netsockstream_readoptions_t &operator=( netsockstream_readoptions_t && ) noexcept = default;
						};

						struct ssl_params_t {
							void set_verify_mode( );
							void set_verify_callback( );
						};

						// Data members
						BoostSocket m_socket;
						std::shared_ptr<daw::nodepp::base::Semaphore<int>> m_pending_writes;
						daw::nodepp::base::data_t m_response_buffers;
						std::size_t m_bytes_read;
						std::size_t m_bytes_written;
						netsockstream_readoptions_t m_read_options;
						netsockstream_state_t m_state;

						explicit NetSocketStreamImpl( base::EventEmitter emitter );

						NetSocketStreamImpl( std::shared_ptr<EncryptionContext> ctx, base::EventEmitter emitter );

						NetSocketStreamImpl( SslServerConfig const &ssl_config, base::EventEmitter emitter );

						friend daw::nodepp::lib::net::NetSocketStream
						daw::nodepp::lib::net::create_net_socket_stream( base::EventEmitter emitter );

						friend daw::nodepp::lib::net::NetSocketStream
						daw::nodepp::lib::net::create_net_socket_stream( SslServerConfig const &ssl_config,
						                                                 base::EventEmitter emitter );

					public:
						NetSocketStreamImpl( ) = delete;
						~NetSocketStreamImpl( ) override;
						NetSocketStreamImpl( NetSocketStreamImpl const & ) = delete;
						NetSocketStreamImpl( NetSocketStreamImpl && ) noexcept = default;
						NetSocketStreamImpl &operator=( NetSocketStreamImpl const & ) = delete;
						NetSocketStreamImpl &operator=( NetSocketStreamImpl && ) noexcept = default;

						NetSocketStreamImpl &
						read_async( std::shared_ptr<daw::nodepp::base::stream::StreamBuf> read_buffer = nullptr );
						daw::nodepp::base::data_t read( );
						daw::nodepp::base::data_t read( std::size_t bytes );

						NetSocketStreamImpl &async_write( daw::nodepp::base::data_t const &chunk );
						NetSocketStreamImpl &
						write_async( daw::string_view chunk,
						             daw::nodepp::base::Encoding const &encoding = daw::nodepp::base::Encoding( ) );

						NetSocketStreamImpl &write( base::data_t const &chunk );
						NetSocketStreamImpl &write( string_view chunk, base::Encoding const &enc );
						NetSocketStreamImpl &write( string_view chunk );

						template<typename BytePtr>
						NetSocketStreamImpl &write( BytePtr first, BytePtr const last ) {
							emit_error_on_throw( get_ptr( ), "Exception while writing byte stream",
							                     "NetSocketStreamImpl::write<BytePtr>", [&]( ) {
								                     auto const dist = std::distance( first, last );
								                     if( dist == 0 ) {
									                     return;
								                     }
								                     daw::exception::daw_throw_on_false( dist > 0, "first must preceed last" );

								                     daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
								                                                        "Attempt to use a closed NetSocketStreamImpl" );

								                     boost::asio::const_buffers_1 buff{first, dist};
								                     m_socket.write( buff );
							                     } );
							return *this;
						}

						template<typename BytePtr>
						NetSocketStreamImpl &async_write( BytePtr first, BytePtr const last ) {
							emit_error_on_throw(
							  get_ptr( ), "Exception while writing byte stream", "NetSocketStreamImpl::async_write<BytePtr>", [&]( ) {
								  auto const dist = std::distance( first, last );
								  if( dist == 0 ) {
									  return;
								  }
								  daw::exception::daw_throw_on_false( dist > 0, "first must preceed last" );
								  daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
								                                     "Attempt to use a closed NetSocketStreamImpl" );

								  auto data = std::make_shared<std::vector<uint8_t>>( );
								  daw::exception::daw_throw_on_false( data, "Could not create data buffer" );
								  data->reserve( dist );
								  std::copy( first, last, std::back_inserter( *data ) );
								  auto buff = std::make_shared<boost::asio::const_buffers_1>( data->data( ), data->size( ) );
								  daw::exception::daw_throw_on_false( buff, "Could not create buffer" );

								  m_pending_writes->inc_counter( );
								  auto obj = this->get_weak_ptr( );
								  auto outstanding_writes = m_pending_writes->get_weak_ptr( );

								  m_socket.async_write( *buff, [outstanding_writes, obj, buff,
								                                data]( base::ErrorCode const &err, size_t bytes_transfered ) mutable {
									  handle_write( outstanding_writes, obj, err, bytes_transfered );
								  } );
							  } );

							return *this;
						}

						NetSocketStreamImpl &send_file( string_view file_name );
						NetSocketStreamImpl &async_send_file( string_view file_name );

						NetSocketStreamImpl &end( );
						NetSocketStreamImpl &end( daw::nodepp::base::data_t const &chunk );
						NetSocketStreamImpl &end( daw::string_view chunk,
						                          daw::nodepp::base::Encoding const &encoding = daw::nodepp::base::Encoding( ) );

						NetSocketStreamImpl &connect( daw::string_view host, uint16_t port );

						void close( bool emit_cb = true );
						void cancel( );

						bool is_open( ) const;
						bool is_closed( ) const;
						bool can_write( ) const;

						NetSocketStreamImpl &set_read_mode( NetSocketStreamReadMode mode );
						NetSocketStreamReadMode const &current_read_mode( ) const;
						NetSocketStreamImpl &set_read_predicate(
						  std::function<std::pair<NetSocketStreamImpl::match_iterator_t, bool>(
						    NetSocketStreamImpl::match_iterator_t begin, NetSocketStreamImpl::match_iterator_t end )>
						    match_function );
						NetSocketStreamImpl &clear_read_predicate( );
						NetSocketStreamImpl &set_read_until_values( std::string values, bool is_regex );

						daw::nodepp::lib::net::impl::BoostSocket &socket( );
						daw::nodepp::lib::net::impl::BoostSocket const &socket( ) const;

						std::size_t &buffer_size( );

						NetSocketStreamImpl &set_timeout( int32_t value );

						NetSocketStreamImpl &set_no_delay( bool noDelay );
						NetSocketStreamImpl &set_keep_alive( bool keep_alive, int32_t initial_delay );

						std::string remote_address( ) const;
						std::string local_address( ) const;
						uint16_t remote_port( ) const;
						uint16_t local_port( ) const;

						std::size_t bytes_read( ) const;
						std::size_t bytes_written( ) const;

						//////////////////////////////////////////////////////////////////////////
						/// Callbacks

						//////////////////////////////////////////////////////////////////////////
						/// Summary: Event emitted when a connection is established
						template<typename Listener>
						NetSocketStreamImpl &on_connected( Listener listener ) {
							auto cb = [ obj = this->get_weak_ptr( ), listener = std::move( listener ) ]( ) mutable {
								if( auto self = obj.lock( ) ) {
									listener( self );
								}
							};
							this->emitter( )->template add_listener<NetSocketStream>( "connect", std::move( cb ) );
							return *this;
						}

						//////////////////////////////////////////////////////////////////////////
						/// Summary: Event emitted when a connection is established
						template<typename Listener>
						NetSocketStreamImpl &on_next_connected( Listener listener ) {
							this->emitter( )->template add_listener<NetSocketStream>(
							  "connect", [ obj = this->get_weak_ptr( ), listener = std::move( listener ) ]( ) {
								  if( auto self = obj.lock( ) ) {
									  listener( self );
								  }
							  },
							  callback_runmode_t::run_once );
							return *this;
						}
						//////////////////////////////////////////////////////////////////////////
						/// StreamReadable

						void emit_connect( );
						void emit_timeout( );

					private:
						static void handle_connect( std::weak_ptr<NetSocketStreamImpl> obj, base::ErrorCode const &err );

						static void handle_read( std::weak_ptr<NetSocketStreamImpl> obj,
						                         std::shared_ptr<daw::nodepp::base::stream::StreamBuf> read_buffer,
						                         base::ErrorCode const &err, std::size_t const &bytes_transferred );
						static void handle_write( std::weak_ptr<daw::nodepp::base::Semaphore<int>> outstanding_writes,
						                          std::weak_ptr<NetSocketStreamImpl> obj, daw::nodepp::base::write_buffer buff,
						                          base::ErrorCode const &err, size_t const &bytes_transferred );

						static void handle_write( std::weak_ptr<daw::nodepp::base::Semaphore<int>> outstanding_writes,
						                          std::weak_ptr<NetSocketStreamImpl> obj, base::ErrorCode const &err,
						                          size_t const &bytes_transfered );

						void async_write( daw::nodepp::base::write_buffer buff );

						void write( base::write_buffer buff );

					}; // struct NetSocketStreamImpl

					void set_ipv6_only( std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor,
					                    daw::nodepp::lib::net::ip_version ip_ver );
				} // namespace impl

				NetSocketStream &operator<<( NetSocketStream &socket, daw::string_view message );

			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
