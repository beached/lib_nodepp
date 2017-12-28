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

#include <daw/daw_bit.h>
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

				enum class NetSocketStreamReadMode : uint_fast8_t {
					newline,
					buffer_full,
					predicate,
					next_byte,
					regex,
					values,
					double_newline
				};

				namespace impl {
					struct netsockstream_state_t {
						using flag_t = uint8_t;
						flag_t state_flags;
						enum : flag_t { closed_flag = 0, end_flag = 1 };

						constexpr bool closed( ) const noexcept {
							return daw::get_bits( state_flags, closed_flag ) != 0;
						}

						constexpr void closed( bool b ) noexcept {
							if( b ) {
								state_flags = daw::set_bits( state_flags, closed_flag );
							} else {
								state_flags = daw::unset_bits( state_flags, closed_flag );
							}
						}

						constexpr bool end( ) const noexcept {
							return daw::get_bits( state_flags, end_flag ) != 0;
						}

						constexpr void end( bool b ) noexcept {
							if( b ) {
								state_flags = daw::set_bits( state_flags, end_flag );
							} else {
								state_flags = daw::unset_bits( state_flags, end_flag );
							}
						}

						constexpr netsockstream_state_t( ) noexcept
						  : state_flags{0} {}

						~netsockstream_state_t( ) = default;

						constexpr netsockstream_state_t( netsockstream_state_t const & ) noexcept = default;

						constexpr netsockstream_state_t( netsockstream_state_t && ) noexcept = default;

						constexpr netsockstream_state_t &operator=( netsockstream_state_t const & ) noexcept = default;

						constexpr netsockstream_state_t &operator=( netsockstream_state_t && ) noexcept = default;
					};

					using match_iterator_t = boost::asio::buffers_iterator<base::stream::StreamBuf::const_buffers_type>;
					using match_function_t =
					  std::function<std::pair<match_iterator_t, bool>( match_iterator_t begin, match_iterator_t end )>;

					struct netsockstream_readoptions_t {
						size_t max_read_size = 8192;
						std::unique_ptr<match_function_t> read_predicate;
						std::string read_until_values;
						NetSocketStreamReadMode read_mode = NetSocketStreamReadMode::newline;

						netsockstream_readoptions_t( ) = default;

						~netsockstream_readoptions_t( ) = default;

						explicit netsockstream_readoptions_t( size_t max_read_size_ )
						  : max_read_size{max_read_size_}
						  , read_mode{NetSocketStreamReadMode::newline} {}

						netsockstream_readoptions_t( netsockstream_readoptions_t const & ) = delete;

						netsockstream_readoptions_t( netsockstream_readoptions_t && ) noexcept = default;

						netsockstream_readoptions_t &operator=( netsockstream_readoptions_t const & ) = delete;

						netsockstream_readoptions_t &operator=( netsockstream_readoptions_t && ) noexcept = default;
					};
				} // namespace impl

				class NetSocketStream : public daw::nodepp::base::SelfDestructing<NetSocketStream>,
				                        public daw::nodepp::base::stream::StreamReadableEvents<NetSocketStream>,
				                        public daw::nodepp::base::stream::StreamWritableEvents<NetSocketStream> {

					struct ssl_params_t {
						void set_verify_mode( );
						void set_verify_callback( );
					};

					// Data members
					struct ss_data_t {
						impl::BoostSocket m_socket;
						daw::observable_ptr<daw::nodepp::base::Semaphore<int>> m_pending_writes;
						daw::nodepp::base::data_t m_response_buffers;
						std::size_t m_bytes_read;
						std::size_t m_bytes_written;
						impl::netsockstream_readoptions_t m_read_options;
						impl::netsockstream_state_t m_state;
					};
					boost::variant<daw::observable_ptr<ss_data_t>, daw::observer_ptr<ss_data_t>> m_data;

					daw::observer_ptr<ss_data_t> get_data_observer( );

					NetSocketStream( std::unique_ptr<EncryptionContext> ctx, base::EventEmitter emitter );
					NetSocketStream( daw::observer_ptr<ss_data_t> data );

				public:
					NetSocketStream( ) = delete;

					explicit NetSocketStream( base::EventEmitter emitter = base::EventEmitter{} );
					explicit NetSocketStream( SslServerConfig const &ssl_config,
					                          base::EventEmitter emitter = base::EventEmitter{} );

					~NetSocketStream( ) override;
					NetSocketStream( NetSocketStream && ) noexcept = default;
					NetSocketStream &operator=( NetSocketStream && ) noexcept = default;
					NetSocketStream( NetSocketStream const &other );
					NetSocketStream &operator=( NetSocketStream const &rhs );

					NetSocketStream &
					read_async( daw::observable_ptr<daw::nodepp::base::stream::StreamBuf> read_buffer = nullptr );
					daw::nodepp::base::data_t read( );
					daw::nodepp::base::data_t read( std::size_t bytes );

					bool expired( );

					template<typename BytePtr, daw::required<( sizeof( *std::declval<BytePtr>( ) ) == 1 )> = nullptr>
					NetSocketStream &write( BytePtr first, BytePtr last ) {
						emit_error_on_throw( emitter( ).get_observer( ), "Exception while writing byte stream",
						                     "NetSocketStream::write<BytePtr>", [&]( ) {
							                     auto const dist = std::distance( first, last );
							                     if( dist < 1 ) {
								                     daw::exception::daw_throw_on_false( dist == 0, "first must preceed last" );
								                     return;
							                     }
							                     daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
							                                                        "Attempt to use a closed NetSocketStream" );

							                     boost::asio::const_buffers_1 buff{static_cast<void const *>( &( *first ) ),
							                                                       static_cast<size_t>( dist )};
							                     boost::apply_visitor(
							                       [&]( auto &ptr ) {
								                       auto lck = ptr.borrow( );
								                       lck->m_socket.write( buff );
							                       },
							                       m_data );
						                     } );
						return *this;
					}

					template<size_t N>
					NetSocketStream &write( char const ( &ptr )[N] ) {
						static_assert( N > 0, "Unexpected empty char array" );
						return write( ptr, ptr + ( N - 1 ) );
					}

					template<typename Container,
					         std::enable_if_t<daw::traits::is_container_like_v<Container>, std::nullptr_t> = nullptr>
					NetSocketStream &write( Container &&container ) {
						static_assert( sizeof( *std::cbegin( container ) ), "Data in container must be byte sized" );
						return write( std::cbegin( container ), std::cend( container ) );
					}

					template<typename BytePtr>
					NetSocketStream &write_async( BytePtr first, BytePtr const last ) {
						static_assert( sizeof( *first ) == 1, "BytePtr must be byte sized" );
						emit_error_on_throw(
						  emitter( ).get_observer( ), "Exception while writing byte stream",
						  "NetSocketStream::write_async<BytePtr>", [&]( ) {
							  auto const dist = std::distance( first, last );
							  if( dist == 0 ) {
								  return;
							  }
							  daw::exception::daw_throw_on_false( dist > 0, "first must preceed last" );
							  daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
							                                     "Attempt to use a closed NetSocketStream" );

							  auto data = daw::make_observable_ptr<std::vector<uint8_t>>( );
							  daw::exception::daw_throw_on_false( data, "Could not create data buffer" );
							  data->reserve( static_cast<size_t>( dist ) );
							  std::copy( first, last, std::back_inserter( *data ) );
							  auto buff = daw::make_observable_ptr<boost::asio::const_buffers_1>( data->data( ), data->size( ) );
							  daw::exception::daw_throw_on_false( buff, "Could not create buffer" );
							  boost::apply_visitor(
							    [&]( auto &ptr ) {
								    auto lck = ptr.borrow( );
								    lck->m_pending_writes->inc_counter( );
								    auto outstanding_writes = lck->m_pending_writes.get_observer( );

								    lck->m_socket.write_async( *buff, [ outstanding_writes, obj = obs_emitter( ), buff, data ](
								                                        base::ErrorCode const &err, size_t bytes_transfered ) mutable {
									    handle_write( outstanding_writes, obj, err, bytes_transfered );
								    } );
							    },
							    m_data );

						  } );

						return *this;
					}

					template<size_t N>
					NetSocketStream &write_async( char const ( &ptr )[N] ) {
						static_assert( N > 0, "Unexpected empty char array" );
						return write_async( ptr, ptr + ( N - 1 ) );
					}

					template<typename Container,
					         std::enable_if_t<daw::traits::is_container_like_v<Container>, std::nullptr_t> = nullptr>
					NetSocketStream &write_async( Container const &container ) {
						return this->write_async( std::cbegin( container ), std::cend( container ) );
					}

					NetSocketStream &send_file( string_view file_name );
					NetSocketStream &send_file_async( string_view file_name );

					NetSocketStream &end( );

					template<typename... Args, std::enable_if_t<( sizeof...( Args ) > 0 ), std::nullptr_t> = nullptr>
					NetSocketStream &end( Args &&... args ) {
						this->write_async( std::forward<Args>( args )... );
						return this->end( );
					}

					NetSocketStream &connect( daw::string_view host, uint16_t port );

					void close( bool emit_cb = true );
					void cancel( );

					bool is_open( ) const;
					bool is_closed( ) const;
					bool can_write( ) const;

					NetSocketStream &set_read_mode( NetSocketStreamReadMode mode );
					NetSocketStreamReadMode const &current_read_mode( ) const;

					NetSocketStream &
					set_read_predicate( std::function<std::pair<impl::match_iterator_t, bool>( impl::match_iterator_t begin,
					                                                                           impl::match_iterator_t end )>
					                      match_function );

					NetSocketStream &clear_read_predicate( );
					NetSocketStream &set_read_until_values( std::string values, bool is_regex );

					daw::nodepp::lib::net::impl::BoostSocket &socket( );
					daw::nodepp::lib::net::impl::BoostSocket const &socket( ) const;

					std::size_t &buffer_size( );

					NetSocketStream &set_timeout( int32_t value );

					NetSocketStream &set_no_delay( bool noDelay );
					NetSocketStream &set_keep_alive( bool keep_alive, int32_t initial_delay );

					std::string remote_address( ) const;
					std::string local_address( ) const;
					uint16_t remote_port( ) const;
					uint16_t local_port( ) const;

					std::size_t bytes_read( ) const;
					std::size_t bytes_written( ) const;

					//////////////////////////////////////////////////////////////////////////
					/// Callbacks

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when a connection is established
					template<typename Listener>
					NetSocketStream &on_connected( Listener listener ) {
						auto cb = [&, listener = std::move( listener ) ]( ) mutable {
							listener( *this );
						};
						this->emitter( )->template add_listener<NetSocketStream>( "connect", std::move( cb ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when a connection is established
					template<typename Listener>
					NetSocketStream &on_next_connected( Listener listener ) {
						this->emitter( )->template add_listener<NetSocketStream>(
						  "connect", [ obj = get_data_observer( ), listener = std::move( listener ) ]( ) {
							  if( obj.expired( ) ) {
								  return;
							  }
							  NetSocketStream nss{obj};
							  listener( nss );
						  },
						  callback_runmode_t::run_once );
						return *this;
					}

					NetSocketStream &write_async( daw::nodepp::base::write_buffer buff );

					NetSocketStream &write( base::write_buffer buff );

					//////////////////////////////////////////////////////////////////////////
					/// StreamReadable

					void emit_connect( );
					void emit_timeout( );

				private:
					static void handle_connect( daw::observer_ptr<NetSocketStream> obj, base::ErrorCode const &err );

					static void handle_read( daw::observer_ptr<NetSocketStream> obj,
					                         daw::observable_ptr<daw::nodepp::base::stream::StreamBuf> read_buffer,
					                         base::ErrorCode const &err, std::size_t const &bytes_transferred );
					static void handle_write( daw::observer_ptr<daw::nodepp::base::Semaphore<int>> outstanding_writes,
					                          daw::observer_ptr<NetSocketStream> obj, daw::nodepp::base::write_buffer buff,
					                          base::ErrorCode const &err, size_t const &bytes_transferred );

					static void handle_write( daw::observer_ptr<daw::nodepp::base::Semaphore<int>> outstanding_writes,
					                          daw::observer_ptr<NetSocketStream> obj, base::ErrorCode const &err,
					                          size_t const &bytes_transfered );

				}; // struct NetSocketStream

				void set_ipv6_only( daw::observable_ptr<boost::asio::ip::tcp::acceptor> acceptor,
				                    daw::nodepp::lib::net::ip_version ip_ver );

				NetSocketStream &operator<<( NetSocketStream &socket, daw::string_view message );

			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
