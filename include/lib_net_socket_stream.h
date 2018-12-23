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

#include <asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include <daw/daw_bit.h>
#include <daw/daw_string_view.h>

#include "base_enoding.h"
#include "base_error.h"
#include "base_selfdestruct.h"
#include "base_service_handle.h"
#include "base_stream.h"
#include "base_types.h"
#include "base_write_buffer.h"
#include "lib_net_dns.h"
#include "lib_net_socket_asio_socket.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using EndPoint = asio::ip::tcp::endpoint;

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
					base::data_t get_clear_buffer( base::data_t &original_buffer,
					                               size_t num_items,
					                               size_t new_size = 1024 );

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

					using match_iterator_t =
					  asio::buffers_iterator<base::stream::StreamBuf::const_buffers_type>;

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

				template<typename EventEmitter>
				class NetSocketStream
				  : public base::SelfDestructing<NetSocketStream<EventEmitter>,
				                                 EventEmitter>,
				    public base::stream::StreamWritableEvents<
				      NetSocketStream<EventEmitter>> {

					struct ssl_params_t {
						void set_verify_mode( );
						void set_verify_callback( );
					};

					// Data members
					struct ss_data_t {
						impl::BoostSocket m_socket{};
						std::atomic_int m_pending_writes{};
						base::data_t m_response_buffers{};
						std::size_t m_bytes_read{};
						std::size_t m_bytes_written{};
						impl::netsockstream_readoptions_t m_read_options{};
						impl::netsockstream_state_t m_state{};

						ss_data_t( ) noexcept = default;

						explicit ss_data_t( SslServerConfig const &ssl_config )
						  : m_socket( ssl_config )
						  , m_pending_writes( 0 )
						  , m_response_buffers( )
						  , m_bytes_read( 0 )
						  , m_bytes_written( 0 )
						  , m_read_options( )
						  , m_state( ) {}

						explicit ss_data_t( std::unique_ptr<asio::ssl::context> ctx )
						  : m_socket( daw::move( ctx ) )
						  , m_pending_writes( 0 )
						  , m_response_buffers( )
						  , m_bytes_read( 0 )
						  , m_bytes_written( 0 )
						  , m_read_options( )
						  , m_state( ) {}
					};
					std::shared_ptr<ss_data_t> m_data;

				public:
					NetSocketStream( NetSocketStream const & ) = default;
					NetSocketStream( NetSocketStream && ) noexcept = default;
					NetSocketStream &operator=( NetSocketStream const & ) = default;
					NetSocketStream &operator=( NetSocketStream && ) noexcept = default;

					~NetSocketStream( ) noexcept {
						try {
							try {
								if( m_data ) {
									if( m_data->m_socket.is_open( ) ) {
										auto ec = base::ErrorCode( );
										m_data->m_socket.shutdown( ec );
										m_data->m_socket.close( ec );
									}
								}
							} catch( ... ) {
								this->emit_error( std::current_exception( ),
								                  "Error shutting down socket ~NetSocketStream",
								                  "NetSocketStream::~NetSocketStream" );
							}
						} catch( ... ) {}
					}

					explicit NetSocketStream( EventEmitter &&emitter = EventEmitter( ) )
					  : base::SelfDestructing<NetSocketStream<EventEmitter>,
					                          EventEmitter>( daw::move( emitter ) )
					  , m_data( std::make_shared<ss_data_t>( ) ) {}

					explicit NetSocketStream( SslServerConfig const &ssl_config,
					                          EventEmitter &&emitter = EventEmitter( ) )
					  : base::SelfDestructing<NetSocketStream<EventEmitter>,
					                          EventEmitter>( daw::move( emitter ) )
					  , m_data( std::make_shared<ss_data_t>( ssl_config ) ) {}

					base::data_t read( ) {
						return impl::get_clear_buffer( m_data->m_response_buffers,
						                               m_data->m_response_buffers.size( ),
						                               0 );
					}

					base::data_t read( size_t /*bytes*/ ) {
						daw::exception::daw_throw_not_implemented( );
					}

					bool expired( ) const {
						// TOOD verify I didn't switch logic
						return !static_cast<bool>( m_data );
					}

					explicit operator bool( ) const {
						return static_cast<bool>( m_data );
					}

					void write( base::data_t const &data ) {
						m_data->m_socket.write( asio::buffer( data ) );
					}

					template<typename BytePtr>
					NetSocketStream &write( BytePtr first, BytePtr last ) {
						static_assert( sizeof( *std::declval<BytePtr>( ) ) == 1,
						               "Expecting byte sized data" );
						try {
							auto const dist = std::distance( first, last );
							daw::exception::daw_throw_on_true(
							  is_closed( ) or !can_write( ),
							  "Attempt to use a closed NetSocketStream" );

							asio::const_buffers_1 buff{
							  static_cast<void const *>( &( *first ) ),
							  static_cast<size_t>( dist )};
							write( buff );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception while writing byte stream",
							                  "write<BytePtr>" );
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

					template<typename BytePtr>
					NetSocketStream &write_async( BytePtr first, BytePtr const last ) {
						static_assert( sizeof( *first ) == 1,
						               "BytePtr must be byte sized" );
						try {
							auto const dist = std::distance( first, last );
							if( dist == 0 ) {
								return *this;
							}
							daw::exception::precondition_check(
							  !is_closed( ) && can_write( ),
							  "Attempt to use a closed NetSocketStream" );

							auto buff_data =
							  std::make_unique<std::vector<uint8_t>>( first, last );

							auto buff =
							  asio::const_buffer( buff_data->data( ), buff_data->size( ) );
							++m_data->m_pending_writes;

							m_data->m_socket.write_async(
							  daw::move( buff ),
							  [obj = mutable_capture( *this ), buff_data = daw::move( buff_data )](
							    base::ErrorCode const &err, size_t bytes_transfered ) {
								  handle_write( *obj, err, bytes_transfered );
							  } );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception while writing byte stream",
							                  "write_async<BytePtr>" );
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

					NetSocketStream &send_file( daw::string_view file_name ) {
						try {
							daw::exception::daw_throw_on_true(
							  is_closed( ) or !can_write( ),
							  "Attempt to use a closed NetSocketStream" );

							m_data->m_bytes_written += boost::filesystem::file_size(
							  boost::filesystem::path{file_name.data( )} );
							daw::filesystem::memory_mapped_file_t<char> mmf{file_name};
							daw::exception::daw_throw_on_false( mmf, "Could not open file" );
							asio::const_buffers_1 buff{mmf.data( ), mmf.size( )};
							m_data->m_socket.write( buff );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception while writing from file",
							                  "send_file" );
						}
						return *this;
					}

					NetSocketStream &send_file_async( string_view file_name ) {
						try {
							daw::exception::daw_throw_on_true(
							  is_closed( ) or !can_write( ),
							  "Attempt to use a closed NetSocketStream" );

							auto mmf =
							  std::make_shared<daw::filesystem::memory_mapped_file_t<char>>(
							    file_name );
							daw::exception::daw_throw_on_false( mmf, "Could not open file" );
							daw::exception::daw_throw_on_false( *mmf, "Could not open file" );

							auto buff = std::make_shared<asio::const_buffers_1>(
							  mmf->data( ), mmf->size( ) );
							daw::exception::daw_throw_on_false( buff,
							                                    "Could not create buffer" );

							++m_data->m_pending_writes;
							m_data->m_socket.write_async(
							  *buff, [obj = mutable_capture( *this ), buff,
							          mmf]( base::ErrorCode err, size_t bytes_transfered ) {
								  handle_write( *obj, err, bytes_transfered );
							  } );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception while writing from file",
							                  "send_file_async" );
						}
						return *this;
					}

					NetSocketStream &end( ) {
						try {
							m_data->m_state.end( true );
							if( m_data->m_socket.is_open( ) ) {
								m_data->m_socket.shutdown( );
							}
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception calling shutdown on socket", "end" );
						}
						return *this;
					}

					template<typename... Args, std::enable_if_t<( sizeof...( Args ) > 0 ),
					                                            std::nullptr_t> = nullptr>
					NetSocketStream &end( Args &&... args ) {
						this->write_async( std::forward<Args>( args )... );
						return this->end( );
					}

					NetSocketStream &connect( daw::string_view host, uint16_t port ) {

						try {
							auto resolver =
							  asio::ip::tcp::resolver( base::ServiceHandle::get( ) );
							auto r =
							  resolver.resolve( {host.to_string( ), std::to_string( port )} );
							// TODO ensure we have the correct handling, not passing endpoint
							// on
							auto handler = [obj = mutable_capture( *this )](
							                 daw::nodepp::base::ErrorCode const &err,
							                 auto && ) { handle_connect( *obj, err ); };

							m_data->m_socket.connect_async( daw::move( r ),
							                                daw::move( handler ) );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception starting connect", "connect" );
						}
						return *this;
					}

					void close( bool emit_cb = true ) {
						try {
							m_data->m_state.closed( true );
							m_data->m_state.end( true );
							if( m_data->m_socket.is_open( ) ) {
								m_data->m_socket.cancel( );
								m_data->m_socket.reset_socket( );
							}
							if( emit_cb ) {
								emit_closed( );
							}
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception while closing socket", "close" );
						}
					}

					void cancel( ) {
						m_data->m_socket.cancel( );
					}

					bool is_closed( ) const {
						return m_data->m_state.closed( );
					}

					bool is_open( ) const {
						return m_data->m_socket.is_open( );
					}

					bool can_write( ) const {
						return !m_data->m_state.end( );
					}

					template<typename ReadPredicate>
					NetSocketStream &
					set_read_predicate( ReadPredicate &&read_predicate ) {
						static_assert(
						  std::is_invocable_r_v<std::pair<impl::match_iterator_t, bool>,
						                        ReadPredicate, impl::match_iterator_t,
						                        impl::match_iterator_t>,
						  "ReadPredicate does not fullfill a match_function_t" );

						m_data->m_read_options.read_predicate =
						  std::make_unique<impl::match_function_t>(
						    std::forward<ReadPredicate>( read_predicate ) );

						m_data->m_read_options.read_mode =
						  NetSocketStreamReadMode::predicate;
						return *this;
					}

					NetSocketStream &set_read_mode( NetSocketStreamReadMode mode ) {
						m_data->m_read_options.read_mode = mode;
						return *this;
					}

					NetSocketStreamReadMode current_read_mode( ) const {
						return m_data->m_read_options.read_mode;
					}

					NetSocketStream &clear_read_predicate( ) {
						auto ptr = m_data.borrow( );

						if( NetSocketStreamReadMode::predicate ==
						    ptr->m_read_options.read_mode ) {
							ptr->m_read_options.read_mode = NetSocketStreamReadMode::newline;
						}
						ptr->m_read_options.read_until_values.clear( );
						ptr->m_read_options.read_predicate.reset( );
						return *this;
					}

					NetSocketStream &set_read_until_values( std::string values,
					                                        bool is_regex ) {
						m_data->m_read_options.read_mode =
						  is_regex ? NetSocketStreamReadMode::regex
						           : NetSocketStreamReadMode::values;
						m_data->m_read_options.read_until_values = daw::move( values );
						m_data->m_read_options.read_predicate.reset( );
						return *this;
					}

					void emit_connect( ) {
						this->emitter( ).emit( "connect" );
					}

					void emit_timeout( ) {
						this->emitter( ).emit( "timeout" );
					}

					/// Asynchronously read data from a socket
					/// \param read_buffer A shared buffer to write data to as it is
					/// received \return A reference to the socket
					NetSocketStream &
					read_async( std::shared_ptr<daw::nodepp::base::stream::StreamBuf>
					              read_buffer ) {
						try {
							if( !m_data or m_data->m_state.closed( ) ) {
								return *this;
							}
							if( !read_buffer ) {
								read_buffer =
								  std::make_shared<daw::nodepp::base::stream::StreamBuf>(
								    m_data->m_read_options.max_read_size );
							}
							auto buff_ptr = read_buffer.get( );
							auto handler = [obj = mutable_capture( *this ),
							                read_buffer =
							                  mutable_capture( daw::move( read_buffer ) )](
							                 base::ErrorCode err, size_t bytes_transfered ) {
								handle_read( *obj, daw::move( *read_buffer ), err,
								             bytes_transfered );
							};
							static boost::regex const dbl_newline( R"((?:\r\n|\n){2})" );

							switch( m_data->m_read_options.read_mode ) {
							case NetSocketStreamReadMode::next_byte:
								daw::exception::daw_throw_not_implemented( );
							case NetSocketStreamReadMode::buffer_full:
								m_data->m_socket.read_async( *buff_ptr, handler );
								break;
							case NetSocketStreamReadMode::newline:
								m_data->m_socket.read_until_async( *buff_ptr, "\n", handler );
								break;
							case NetSocketStreamReadMode::double_newline:
								m_data->m_socket.read_until_async( *buff_ptr, dbl_newline,
								                                   handler );
								break;
							case NetSocketStreamReadMode::predicate:
								m_data->m_socket.read_until_async(
								  *buff_ptr, *m_data->m_read_options.read_predicate, handler );
								break;
							case NetSocketStreamReadMode::values:
								m_data->m_socket.read_until_async(
								  *buff_ptr, m_data->m_read_options.read_until_values,
								  handler );
								break;
							case NetSocketStreamReadMode::regex:
								m_data->m_socket.read_until_async(
								  *buff_ptr,
								  boost::regex( m_data->m_read_options.read_until_values ),
								  handler );
								break;
							default:
								daw::exception::daw_throw_unexpected_enum( );
							}
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception starting async read", "read_async" );
						}
						return *this;
					}

					/// Asynchronously read data from a socket
					/// \return A reference to the socket
					NetSocketStream &read_async( ) {
						return read_async(
						  std::shared_ptr<daw::nodepp::base::stream::StreamBuf>( ) );
					}

					size_t &buffer_size( ) {
						daw::exception::daw_throw_not_implemented( );
					}

					daw::nodepp::lib::net::impl::BoostSocket &socket( ) {
						return m_data->m_socket;
					}

					daw::nodepp::lib::net::impl::BoostSocket const &socket( ) const {
						return m_data->m_socket;
					}

					NetSocketStream &write_async( base::write_buffer buff ) {
						try {
							daw::exception::daw_throw_on_true(
							  is_closed( ) or !can_write( ),
							  "Attempt to use a closed NetSocketStream" );
							m_data->m_bytes_written += buff.size( );

							++m_data->m_pending_writes;
							m_data->m_socket.write_async(
							  buff.asio_buff( ),
							  [obj = mutable_capture( *this ),
							   buff = mutable_capture( daw::move( buff ) )](
							    base::ErrorCode err, size_t bytes_transfered ) {
								  handle_write( *obj, daw::move( *buff ), err,
								                bytes_transfered );
							  } );
						} catch( ... ) {
							this->emit_error( std::current_exception( ),
							                  "Exception while writing", "write_async" );
						}
						return *this;
					}

					NetSocketStream &set_timeout( int32_t ) {
						daw::exception::daw_throw_not_implemented( );
					}

					NetSocketStream &set_no_delay( bool ) {
						daw::exception::daw_throw_not_implemented( );
					}

					NetSocketStream &set_keep_alive( bool, int32_t ) {
						daw::exception::daw_throw_not_implemented( );
					}

					///
					/// \return A string representing the address of the remote host
					std::string remote_address( ) const {
						return m_data->m_socket.remote_endpoint( ).address( ).to_string( );
					}

					std::string local_address( ) const {
						return m_data->m_socket.local_endpoint( ).address( ).to_string( );
					}

					uint16_t remote_port( ) const {
						return m_data->m_socket.remote_endpoint( ).port( );
					}

					uint16_t local_port( ) const {
						return m_data->m_socket.local_endpoint( ).port( );
					}

					size_t bytes_read( ) const {
						return m_data->m_bytes_read;
					}

					size_t bytes_written( ) const {
						return m_data->m_bytes_written;
					}

					//////////////////////////////////////////////////////////////////////////
					/// Callbacks

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when a connection is established
					template<typename Listener>
					NetSocketStream &on_connected( Listener &&listener ) {
						this->emitter( ).template add_listener<NetSocketStream>(
						  "connect", [sock = mutable_capture( *this ),
						              listener = mutable_capture(
						                std::forward<Listener>( listener ) )]( ) {
							  ( *listener )( daw::move( *sock ) );
						  } );

						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when a connection is established
					template<typename Listener>
					NetSocketStream &on_next_connected( Listener &&listener ) {
						base::add_listener<NetSocketStream>(
						  "connect", this->emitter( ),
						  [sock = *this, listener = mutable_capture(
						                   std::forward<Listener>( listener ) )]( ) {
							  if( sock ) {
								  ( *listener )( sock );
							  }
						  },
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// StreamReadable
					//////////////////////////////////////////////////////////////////////////

					/// @brief	Event emitted when data is received
					template<typename Listener>
					NetSocketStream &on_data_received( Listener &&listener ) {
						base::add_listener<std::shared_ptr<base::data_t> /* buffer */,
						                   bool /* eof */>(
						  "data_received", this->emitter( ),
						  std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when data is received
					template<typename Listener>
					NetSocketStream &on_next_data_received( Listener &&listener ) {
						base::add_listener<std::shared_ptr<base::data_t>, bool>(
						  "data_received", this->emitter( ),
						  std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when of of stream is read.
					template<typename Listener>
					NetSocketStream &on_eof( Listener &&listener ) {
						base::add_listener<NetSocketStream>(
						  "eof", this->emitter( ), std::forward<Listener>( listener ) );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Event emitted when of of stream is read.
					template<typename Listener>
					NetSocketStream &on_next_eof( Listener &&listener ) {
						base::add_listener<NetSocketStream>(
						  "eof", this->emitter( ), std::forward<Listener>( listener ),
						  base::callback_run_mode_t::run_once );
						return *this;
					}

					///
					/// \tparam Listener An invokable type that takes no arguments
					/// \param listener Callback that is called when connection is closed
					/// \return A reference to socket
					template<typename Listener>
					NetSocketStream &on_closed( Listener &&listener ) {
						base::add_listener<>( "closed", this->emitter( ),
						                      std::forward<Listener>( listener ) );
						return *this;
					}

					template<typename Listener>
					NetSocketStream &on_next_closed( Listener &&listener ) {
						base::add_listener<>( "closed", this->emitter( ),
						                      std::forward<Listener>( listener ),
						                      base::callback_run_mode_t::run_once );
						return *this;
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief	Emit an event with the data received and whether the eof
					///				has been reached
					void emit_data_received( std::shared_ptr<base::data_t> buffer,
					                         bool end_of_file ) {
						this->emitter( ).emit( "data_received", daw::move( buffer ),
						                       end_of_file );
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when the eof has been reached
					void emit_eof( ) {
						this->emitter( ).emit( "eof" );
					}

					//////////////////////////////////////////////////////////////////////////
					/// @brief Event emitted when the socket is closed
					void emit_closed( ) {
						this->emitter( ).emit( "closed" );
					}

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

				private:
					static void handle_connect( NetSocketStream &obj,
					                            base::ErrorCode err ) {
						if( err ) {
							obj.emit_error( err, "Running connection listeners", "connect" );
							return;
						}
						try {
							obj.emit_connect( );
						} catch( ... ) {
							obj.emit_error( std::current_exception( ),
							                "Exception while running connection listener",
							                "handle_connect" );
						}
					}

					static void
					handle_read( NetSocketStream &obj,
					             std::shared_ptr<base::stream::StreamBuf> read_buffer,
					             base::ErrorCode err, size_t bytes_transferred ) {
						if( static_cast<bool>( err ) and ENOENT != err.value( ) ) {
							// Any error but "no such file/directory"
							obj.emit_error( err, "Error while reading", "handle_read" );
							return;
						}
						try {
							auto ptr = obj.m_data;
							auto &response_buffers = ptr->m_response_buffers;

							read_buffer->commit( bytes_transferred );
							if( bytes_transferred > 0 ) {

								std::istream resp( read_buffer.get( ) );
								auto new_data = std::make_shared<base::data_t>(
								  bytes_transferred, static_cast<char>( 0 ) );

								resp.read( new_data->data( ),
								           static_cast<std::streamsize>( bytes_transferred ) );
								read_buffer->consume( bytes_transferred );
								if( obj.emitter( ).listener_count( "data_received" ) > 0 ) {
									if( !response_buffers.empty( ) ) {
										auto buff = std::make_shared<base::data_t>(
										  response_buffers.cbegin( ), response_buffers.cend( ) );
										ptr->m_response_buffers.clear( );
										obj.emit_data_received( daw::move( buff ), false );
									}
									bool const end_of_file =
									  static_cast<bool>( err ) and ( ENOENT == err.value( ) );
									obj.emit_data_received( new_data, end_of_file );
								} else { // Queue up for a
									ptr->m_response_buffers.insert(
									  ptr->m_response_buffers.cend( ), new_data->cbegin( ),
									  new_data->cend( ) );
								}
								ptr->m_bytes_read += bytes_transferred;
							}
							if( !err and !obj.is_closed( ) ) {
								obj.read_async( daw::move( read_buffer ) );
							}
						} catch( ... ) {
							obj.emit_error( std::current_exception( ),
							                "Exception while handling read", "handle_read" );
						}
					}

					static void handle_write( NetSocketStream &obj,
					                          base::write_buffer buff,
					                          base::ErrorCode err,
					                          size_t bytes_transferred ) {
						if( !obj ) {
							// outstanding_writes.lock( )->dec_counter( );
							return;
						}
						try {
							obj.m_data->m_bytes_written += bytes_transferred;
							if( !err ) {
								obj.emit_write_completion( obj );
							} else {
								obj.emit_error( err, "Error while writing",
								                "NetSocket::handle_write" );
							}
							if( ( --obj.m_data->m_pending_writes ) == 0 ) {
								obj.emit_all_writes_completed( obj );
							}
						} catch( ... ) {
							obj.emit_error( std::current_exception( ),
							                "Exception while handling write",
							                "handle_write" );
						}
					}

					static void handle_write( NetSocketStream &obj, base::ErrorCode err,
					                          size_t bytes_transfered ) {
						if( !obj.m_data ) {
							return;
						}
						try {
							obj.m_data->m_bytes_written += bytes_transfered;
							if( !err ) {
								obj.emit_write_completion( obj );
							} else {
								obj.emit_error( err, "Error while writing",
								                "NetSocket::handle_write" );
							}
							if( ( --obj.m_data->m_pending_writes ) == 0 ) {
								obj.emit_all_writes_completed( obj );
							}
						} catch( ... ) {
							obj.emit_error( std::current_exception( ),
							                "Exception while handling write",
							                "handle_write" );
						}
					}
				}; // namespace net

				void set_ipv6_only( asio::ip::tcp::acceptor &acceptor,
				                    ip_version ip_ver );

				inline constexpr daw::string_view const eol = "\r\n";

				template<typename Emitter>
				NetSocketStream<Emitter> &operator<<( NetSocketStream<Emitter> &socket,
				                                      daw::string_view message ) {
					daw::exception::precondition_check(
					  socket, "Attempt to use a null NetSocketStream" );

					socket.write_async( message );
					return socket;
				}

			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
