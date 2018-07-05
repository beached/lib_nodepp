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

#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <string>

#include <daw/daw_bit.h>
#include <daw/daw_string_view.h>
#include <daw/parallel/daw_observable_ptr_pair.h>

#include "base_enoding.h"
#include "base_error.h"
#include "base_selfdestruct.h"
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

						constexpr netsockstream_state_t(
						  netsockstream_state_t const & ) noexcept = default;

						constexpr netsockstream_state_t(
						  netsockstream_state_t && ) noexcept = default;

						constexpr netsockstream_state_t &
						operator=( netsockstream_state_t const & ) noexcept = default;

						constexpr netsockstream_state_t &
						operator=( netsockstream_state_t && ) noexcept = default;
					};

					using match_iterator_t = boost::asio::buffers_iterator<
					  base::stream::StreamBuf::const_buffers_type>;

					using match_function_t =
					  std::function<std::pair<match_iterator_t, bool>(
					    match_iterator_t begin, match_iterator_t end )>;

					struct netsockstream_readoptions_t {
						size_t max_read_size = 8192;
						std::unique_ptr<match_function_t> read_predicate = nullptr;
						std::string read_until_values = {};
						NetSocketStreamReadMode read_mode =
						  NetSocketStreamReadMode::newline;

						netsockstream_readoptions_t( ) = default;

						explicit netsockstream_readoptions_t( size_t max_read_size_ )
						  : max_read_size( max_read_size_ )
						  , read_mode( NetSocketStreamReadMode::newline ) {}
					};
				} // namespace impl

				class NetSocketStream
				  : public daw::nodepp::base::SelfDestructing<NetSocketStream>,
				    public daw::nodepp::base::stream::StreamWritableEvents<
				      NetSocketStream> {

					struct ssl_params_t {
						void set_verify_mode( );
						void set_verify_callback( );
					};

					// Data members
					struct ss_data_t {
						impl::BoostSocket m_socket;
						std::atomic_int m_pending_writes;
						base::data_t m_response_buffers;
						std::size_t m_bytes_read;
						std::size_t m_bytes_written;
						impl::netsockstream_readoptions_t m_read_options;
						impl::netsockstream_state_t m_state;

						ss_data_t( );
						ss_data_t( SslServerConfig const &ssl_config );
						ss_data_t( std::unique_ptr<boost::asio::ssl::context> ctx );

						ss_data_t( ss_data_t const & ) = default;
						ss_data_t( ss_data_t && ) = default;
						ss_data_t &operator=( ss_data_t const & ) = default;
						ss_data_t &operator=( ss_data_t && ) = default;
						~ss_data_t( );
					};
					daw::observable_ptr_pair<ss_data_t> m_data;

				public:
					NetSocketStream( NetSocketStream const & ) = default;
					NetSocketStream( NetSocketStream && ) noexcept = default;
					NetSocketStream &operator=( NetSocketStream const & ) = default;
					NetSocketStream &operator=( NetSocketStream && ) noexcept = default;

					~NetSocketStream( ) noexcept;

					explicit NetSocketStream(
					  base::EventEmitter &&emitter = base::EventEmitter( ) );

					explicit NetSocketStream(
					  SslServerConfig const &ssl_config,
					  base::EventEmitter &&emitter = base::EventEmitter( ) );

					NetSocketStream &read_async(
					  std::shared_ptr<daw::nodepp::base::stream::StreamBuf> read_buffer =
					    nullptr );

					daw::nodepp::base::data_t read( );
					daw::nodepp::base::data_t read( std::size_t bytes );

					bool expired( ) const;

					explicit operator bool( ) const {
						return static_cast<bool>( m_data );
					}

					void write( base::data_t const &data );

					template<typename BytePtr>
					NetSocketStream &write( BytePtr first, BytePtr last ) {
						static_assert( sizeof( *std::declval<BytePtr>( ) ) == 1,
						               "Expecting byte sized data" );
						try {
							auto const dist = std::distance( first, last );
							daw::exception::daw_throw_on_true(
							  is_closed( ) || !can_write( ),
							  "Attempt to use a closed NetSocketStream" );

							boost::asio::const_buffers_1 buff{
							  static_cast<void const *>( &( *first ) ),
							  static_cast<size_t>( dist )};
							write( buff );
						} catch( ... ) {
							emit_error( std::current_exception( ),
							            "Exception while writing byte stream",
							            "NetSocketStream::write<BytePtr>" );
						}
						return *this;
					}

					template<size_t N>
					NetSocketStream &write( char const ( &ptr )[N] ) {
						static_assert( N > 0, "Unexpected empty char array" );
						return write( ptr, ptr + ( N - 1 ) );
					}

					template<typename Container,
					         std::enable_if_t<daw::traits::is_container_like_v<Container>,
					                          std::nullptr_t> = nullptr>
					NetSocketStream &write( Container &&container ) {
						static_assert( sizeof( *std::cbegin( container ) ),
						               "Data in container must be byte sized" );
						return write( std::cbegin( container ), std::cend( container ) );
					}

					struct null_buffer_exception {};
					template<typename BytePtr>
					NetSocketStream &write_async( BytePtr first, BytePtr const last ) {
						static_assert( sizeof( *first ) == 1,
						               "BytePtr must be byte sized" );
						try {
							auto const dist = std::distance( first, last );
							if( dist == 0 ) {
								return *this;
							}
							daw::exception::daw_throw_on_true(
							  is_closed( ) || !can_write( ),
							  "Attempt to use a closed NetSocketStream" );

							auto buff_data =
							  std::make_shared<std::vector<uint8_t>>( first, last );
							auto buff = std::make_shared<boost::asio::const_buffers_1>(
							  buff_data->data( ), buff_data->size( ) );

							m_data.visit( [&]( auto &data ) {
								++data.m_pending_writes;

								data.m_socket.write_async(
								  *buff, [obj = *this, buff_data = std::move( buff_data ),
								          buff = std::move( buff )](
								           base::ErrorCode const &err,
								           size_t bytes_transfered ) mutable {
									  handle_write( obj, err, bytes_transfered );
								  } );
							} );
						} catch( ... ) {
							emit_error( std::current_exception( ),
							            "Exception while writing byte stream",
							            "NetSocketStream::write_async<BytePtr>" );
						}
						return *this;
					}

					template<size_t N>
					NetSocketStream &write_async( char const ( &ptr )[N] ) {
						static_assert( N > 0, "Unexpected empty char array" );
						return write_async( ptr, ptr + ( N - 1 ) );
					}

					template<typename Container,
					         std::enable_if_t<daw::traits::is_container_like_v<Container>,
					                          std::nullptr_t> = nullptr>
					NetSocketStream &write_async( Container const &container ) {
						return this->write_async( std::cbegin( container ),
						                          std::cend( container ) );
					}

					NetSocketStream &send_file( string_view file_name );
					NetSocketStream &send_file_async( string_view file_name );

					NetSocketStream &end( );

					template<typename... Args, std::enable_if_t<( sizeof...( Args ) > 0 ),
					                                            std::nullptr_t> = nullptr>
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
					NetSocketStreamReadMode current_read_mode( ) const;

					template<typename ReadPredicate>
					NetSocketStream &
					set_read_predicate( ReadPredicate &&read_predicate ) {
						static_assert(
						  daw::is_callable_convertible_v<
						    std::pair<impl::match_iterator_t, bool>, ReadPredicate,
						    impl::match_iterator_t, impl::match_iterator_t>,
						  "ReadPredicate does not fullfill a match_function_t" );

						m_data.visit( [read_predicate = std::forward<ReadPredicate>(
						                 read_predicate )]( ss_data_t &data ) {
							data.m_read_options.read_predicate =
							  std::make_unique<impl::match_function_t>(
							    std::move( read_predicate ) );

							data.m_read_options.read_mode =
							  NetSocketStreamReadMode::predicate;
						} );
						return *this;
					}

					NetSocketStream &clear_read_predicate( );
					NetSocketStream &set_read_until_values( std::string values,
					                                        bool is_regex );

					daw::nodepp::lib::net::impl::BoostSocket &socket( );
					daw::nodepp::lib::net::impl::BoostSocket const &socket( ) const;

					std::size_t &buffer_size( );

					NetSocketStream &set_timeout( int32_t value );

					NetSocketStream &set_no_delay( bool noDelay );
					NetSocketStream &set_keep_alive( bool keep_alive,
					                                 int32_t initial_delay );

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
					NetSocketStream &on_connected( Listener &&listener ) {
						emitter( ).template add_listener<NetSocketStream>(
						  "connect", [sock = *this, listener = std::forward<Listener>(
						                              listener )]( ) mutable {
							  listener( std::move( sock ) );
						  } );

						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when a connection is established
					template<typename Listener>
					NetSocketStream &on_next_connected( Listener &&listener ) {
						emitter( ).template add_listener<NetSocketStream>(
						  "connect",
						  [sock = *this,
						   listener = std::forward<Listener>( listener )]( ) mutable {
							  sock.m_data.apply_visitor( [&]( auto &obj ) {
								  if( obj.expired( ) ) {
									  return;
								  }
								  listener( sock );
							  } );
						  },
						  callback_runmode_t::run_once );
						return *this;
					}

					NetSocketStream &write_async( daw::nodepp::base::write_buffer buff );

					NetSocketStream &write( base::write_buffer buff );

					//////////////////////////////////////////////////////////////////////////
					/// StreamReadable
					//////////////////////////////////////////////////////////////////////////

					/// @brief	Event emitted when data is received
					template<typename Listener>
					NetSocketStream &on_data_received( Listener &&listener ) {
						emitter( )
						  .template add_listener<std::shared_ptr<base::data_t>, bool>(
						    "data_received", std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when data is received
					template<typename Listener>
					NetSocketStream &on_next_data_received( Listener &&listener ) {
						emitter( )
						  .template add_listener<base::shared_data_t /*buffer*/,
						                         bool /*end_of_file*/>(
						    "data_received", std::forward<Listener>( listener ),
						    callback_runmode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when of of stream is read.
					template<typename Listener>
					NetSocketStream &on_eof( Listener &&listener ) {
						emitter( ).template add_listener<NetSocketStream>(
						  "eof", std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when of of stream is read.
					template<typename Listener>
					NetSocketStream &on_next_eof( Listener &&listener ) {
						emitter( ).template add_listener<NetSocketStream>(
						  "eof", std::forward<Listener>( listener ),
						  callback_runmode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when the stream is closed
					template<typename Listener>
					NetSocketStream &on_closed( Listener &&listener ) {
						emitter( ).template add_listener<>(
						  "closed", std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					NetSocketStream &on_next_closed( Listener &&listener ) {
						emitter( ).template add_listener<>(
						  "closed", std::forward<Listener>( listener ),
						  callback_runmode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Emit an event with the data received and whether the eof
					///				has been reached
					void emit_data_received( std::shared_ptr<base::data_t> buffer,
					                         bool end_of_file );

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when the eof has been reached
					void emit_eof( );

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when the socket is closed
					void emit_closed( );

					template<typename StreamWritableObj>
					NetSocketStream &delegate_data_received_to(
					  std::weak_ptr<StreamWritableObj> stream_writable_obj ) {
						on_data_received(
						  [stream_writable_obj]( base::data_t buff, bool eof ) {
							  if( !stream_writable_obj.expired( ) ) {
								  stream_writable_obj.lock( )->write( buff );
							  }
						  } );
						return *this;
					}

					void emit_connect( );
					void emit_timeout( );

				private:
					static void handle_connect( NetSocketStream &obj,
					                            base::ErrorCode err );

					static void
					handle_read( NetSocketStream &obj,
					             std::shared_ptr<base::stream::StreamBuf> read_buffer,
					             base::ErrorCode err, std::size_t bytes_transferred );

					static void handle_write( NetSocketStream &obj,
					                          daw::nodepp::base::write_buffer buff,
					                          base::ErrorCode err,
					                          size_t bytes_transferred );

					static void handle_write( NetSocketStream &obj, base::ErrorCode err,
					                          size_t bytes_transfered );

				}; // struct NetSocketStream

				void set_ipv6_only( boost::asio::ip::tcp::acceptor &acceptor,
				                    ip_version ip_ver );

				NetSocketStream &operator<<( NetSocketStream &socket,
				                             daw::string_view message );

			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
