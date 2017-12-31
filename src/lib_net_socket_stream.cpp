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

#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <condition_variable>
#include <thread>

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>
#include <daw/daw_utility.h>

#include "base_enoding.h"
#include "base_error.h"
#include "base_event_emitter.h"
#include "base_selfdestruct.h"
#include "base_service_handle.h"
#include "base_stream.h"
#include "base_types.h"
#include "base_write_buffer.h"
#include "lib_net_dns.h"
#include "lib_net_socket_stream.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace net {
				using namespace daw::nodepp;
				using namespace boost::asio::ip;

				//////////////////////////////////////////////////////////////////////////
				/// Helpers
				///
				namespace impl {
					base::data_t get_clear_buffer( base::data_t &original_buffer, size_t num_items, size_t new_size = 1024 ) {
						base::data_t new_buffer( new_size, 0 );
						using std::swap;
						swap( new_buffer, original_buffer );
						new_buffer.resize( num_items );
						return new_buffer;
					}
				} // namespace impl

				NetSocketStream::ss_data_t::ss_data_t( )
				  : m_socket{}
				  , m_pending_writes{0}
				  , m_response_buffers{}
				  , m_bytes_read{0}
				  , m_bytes_written{0}
				  , m_read_options{}
				  , m_state{} {}

				NetSocketStream::ss_data_t::ss_data_t( SslServerConfig const &ssl_config )
				  : m_socket{ssl_config}
				  , m_pending_writes{0}
				  , m_response_buffers{}
				  , m_bytes_read{0}
				  , m_bytes_written{0}
				  , m_read_options{}
				  , m_state{} {}

				NetSocketStream::ss_data_t::ss_data_t( std::unique_ptr<boost::asio::ssl::context> ctx )
				  : m_socket{std::move( ctx )}
				  , m_pending_writes{0}
				  , m_response_buffers{}
				  , m_bytes_read{0}
				  , m_bytes_written{0}
				  , m_read_options{}
				  , m_state{} {}

				NetSocketStream::ss_data_t::~ss_data_t( ) = default;

				NetSocketStream::NetSocketStream( base::EventEmitter emitter )
				  : daw::nodepp::base::SelfDestructing<NetSocketStream>{std::move( emitter )}
				  , m_data{daw::make_observable_ptr_pair<ss_data_t>( )} {}

				/*NetSocketStream::NetSocketStream( std::unique_ptr<boost::asio::ssl::context> ctx, base::EventEmitter emitter )
				  : daw::nodepp::base::SelfDestructing<NetSocketStream>{std::move( emitter )}
				  , m_data{daw::make_observable_ptr_pair<ss_data_t>( std::move( ctx )} {}
				*/
				NetSocketStream::NetSocketStream( SslServerConfig const &ssl_config, base::EventEmitter emitter )
				  : daw::nodepp::base::SelfDestructing<NetSocketStream>{std::move( emitter )}
				  , m_data{daw::make_observable_ptr_pair<ss_data_t>( ssl_config )} {}

				NetSocketStream::~NetSocketStream( ) noexcept {
					try {
						try {
							m_data.visit( [&]( ss_data_t &data ) {
								if( data.m_socket.is_open( ) ) {
									base::ErrorCode ec;
									data.m_socket.shutdown( ec );
									data.m_socket.close( ec );
								}
							} );
						} catch( ... ) {
							emit_error( std::current_exception( ), "Error shutting down socket",
							            "NetSocketStream::~NetSocketStream" );
						}
					} catch( ... ) {}
				}

				bool NetSocketStream::expired( ) const {
					return m_data.apply_visitor( []( auto const &ptr ) { return static_cast<bool>( ptr ); } );
				}

				NetSocketStream &NetSocketStream::set_read_mode( NetSocketStreamReadMode mode ) {
					m_data.visit( [&]( ss_data_t &data ) { data.m_read_options.read_mode = mode; } );
					return *this;
				}

				NetSocketStreamReadMode NetSocketStream::current_read_mode( ) const {
					return m_data.visit( [&]( ss_data_t const &data ) { return data.m_read_options.read_mode; } );
				}

				NetSocketStream &NetSocketStream::set_read_predicate( impl::match_function_t read_predicate ) {
					m_data.visit( [&]( ss_data_t &data ) {
						data.m_read_options.read_predicate =
						  std::make_unique<impl::match_function_t>( std::move( read_predicate ) );

						data.m_read_options.read_mode = NetSocketStreamReadMode::predicate;
					} );
					return *this;
				}

				NetSocketStream &NetSocketStream::clear_read_predicate( ) {
					auto ptr = m_data.borrow( );

					if( NetSocketStreamReadMode::predicate == ptr->m_read_options.read_mode ) {
						ptr->m_read_options.read_mode = NetSocketStreamReadMode::newline;
					}
					ptr->m_read_options.read_until_values.clear( );
					ptr->m_read_options.read_predicate.reset( );
					return *this;
				}

				NetSocketStream &NetSocketStream::set_read_until_values( std::string values, bool is_regex ) {
					m_data.visit( [&]( ss_data_t &data ) {
						data.m_read_options.read_mode = is_regex ? NetSocketStreamReadMode::regex : NetSocketStreamReadMode::values;
						data.m_read_options.read_until_values = std::move( values );
						data.m_read_options.read_predicate.reset( );
					} );
					return *this;
				}

				void NetSocketStream::handle_connect( NetSocketStream &obj, base::ErrorCode err ) {
					if( err ) {
						obj.emit_error( err, "Running connection listeners", "NetSocketStream::connect" );
						return;
					}
					try {
						obj.emit_connect( );
					} catch( ... ) {
						obj.emit_error( std::current_exception( ), "Exception while running connection listener",
						                "NetSocketStream::handle_connect" );
					}
				}

				void NetSocketStream::handle_read( NetSocketStream &obj, std::shared_ptr<base::stream::StreamBuf> read_buffer,
				                                   base::ErrorCode err, size_t bytes_transferred ) {
					if( static_cast<bool>( err ) && ENOENT != err.value( ) ) {
						// Any error but "no such file/directory"
						obj.emit_error( err, "Error while reading", "NetSocketStream::handle_read" );
						return;
					}
					try {
						auto ptr = obj.m_data.borrow( );
						auto &response_buffers = ptr->m_response_buffers;

						read_buffer->commit( bytes_transferred );
						if( bytes_transferred > 0 ) {

							std::istream resp( read_buffer.get( ) );
							auto new_data = std::make_shared<base::data_t>( bytes_transferred, static_cast<char>( 0 ) );

							resp.read( new_data->data( ), static_cast<std::streamsize>( bytes_transferred ) );
							read_buffer->consume( bytes_transferred );
							if( obj.emitter( ).listener_count( "data_received" ) > 0 ) {
								if( !response_buffers.empty( ) ) {
									auto buff = std::make_shared<base::data_t>( response_buffers.cbegin( ), response_buffers.cend( ) );
									ptr->m_response_buffers.clear( );
									obj.emit_data_received( std::move( buff ), false );
								}
								bool const end_of_file = static_cast<bool>( err ) && ( ENOENT == err.value( ) );
								obj.emit_data_received( new_data, end_of_file );
							} else { // Queue up for a
								ptr->m_response_buffers.insert( ptr->m_response_buffers.cend( ), new_data->cbegin( ),
								                                new_data->cend( ) );
							}
							ptr->m_bytes_read += bytes_transferred;
						}
						if( !err && !obj.is_closed( ) ) {
							obj.read_async( std::move( read_buffer ) );
						}
					} catch( ... ) {
						obj.emit_error( std::current_exception( ), "Exception while handling read",
						                "NetSocketStream::handle_read" );
					}
				}

				void NetSocketStream::handle_write( NetSocketStream &obj, base::write_buffer buff, base::ErrorCode err,
				                                    size_t bytes_transferred ) {
					if( !obj.m_data ) {
						// outstanding_writes.lock( )->dec_counter( );
						return;
					}
					try {
						obj.m_data.visit( [&]( ss_data_t &data ) {
							data.m_bytes_written += bytes_transferred;
							if( !err ) {
								obj.emit_write_completion( obj );
							} else {
								obj.emit_error( err, "Error while writing", "NetSocket::handle_write" );
							}
							if( ( --data.m_pending_writes ) == 0 ) {
								obj.emit_all_writes_completed( obj );
							}
						} );
					} catch( ... ) {
						obj.emit_error( std::current_exception( ), "Exception while handling write",
						                "NetSocketStream::handle_write" );
					}
				}

				void NetSocketStream::handle_write( NetSocketStream &obj, base::ErrorCode err, size_t bytes_transfered ) {
					if( !obj.m_data ) {
						return;
					}
					try {
						obj.m_data.visit( [&]( ss_data_t &data ) {
							data.m_bytes_written += bytes_transfered;
							if( !err ) {
								obj.emit_write_completion( obj );
							} else {
								obj.emit_error( err, "Error while writing", "NetSocket::handle_write" );
							}
							if( ( --data.m_pending_writes ) == 0 ) {
								obj.emit_all_writes_completed( obj );
							}
						} );
					} catch( ... ) {
						obj.emit_error( std::current_exception( ), "Exception while handling write",
						                "NetSocketStream::handle_write" );
					}
				}

				void NetSocketStream::emit_connect( ) {
					this->emitter( ).emit( "connect" );
				}

				void NetSocketStream::emit_timeout( ) {
					this->emitter( ).emit( "timeout" );
				}

				NetSocketStream &
				NetSocketStream::read_async( std::shared_ptr<daw::nodepp::base::stream::StreamBuf> read_buffer ) {
					try {
						m_data.visit( [&]( ss_data_t &data ) {
							if( data.m_state.closed( ) ) {
								return;
							}
							if( !read_buffer ) {
								read_buffer =
								  std::make_shared<daw::nodepp::base::stream::StreamBuf>( data.m_read_options.max_read_size );
							}
							auto buff_ptr = read_buffer.get( );
							auto handler = [ obj = *this, read_buffer = std::move( read_buffer ) ](
							  base::ErrorCode err, size_t bytes_transfered ) mutable {
								handle_read( obj, std::move( read_buffer ), err, bytes_transfered );
							};
							static boost::regex const dbl_newline( R"((?:\r\n|\n){2})" );

							switch( data.m_read_options.read_mode ) {
							case NetSocketStreamReadMode::next_byte:
								daw::exception::daw_throw_not_implemented( );
							case NetSocketStreamReadMode::buffer_full:
								data.m_socket.read_async( *buff_ptr, handler );
								break;
							case NetSocketStreamReadMode::newline:
								data.m_socket.read_until_async( *buff_ptr, "\n", handler );
								break;
							case NetSocketStreamReadMode::double_newline:
								data.m_socket.read_until_async( *buff_ptr, dbl_newline, handler );
								break;
							case NetSocketStreamReadMode::predicate:
								data.m_socket.read_until_async( *buff_ptr, *data.m_read_options.read_predicate, handler );
								break;
							case NetSocketStreamReadMode::values:
								data.m_socket.read_until_async( *buff_ptr, data.m_read_options.read_until_values, handler );
								break;
							case NetSocketStreamReadMode::regex:
								data.m_socket.read_until_async( *buff_ptr, boost::regex( data.m_read_options.read_until_values ),
								                                handler );
								break;
							default:
								daw::exception::daw_throw_unexpected_enum( );
							}
						} );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception starting async read", "NetSocketStream::read_async" );
					}
					return *this;
				}

				NetSocketStream &NetSocketStream::connect( daw::string_view host, uint16_t port ) {
					tcp::resolver resolver( base::ServiceHandle::get( ) );
					try {
						m_data->m_socket.connect_async(
						  resolver.resolve( {host.to_string( ), std::to_string( port )} ), [obj = *this](
						                                                                     base::ErrorCode err,
						                                                                     tcp::resolver::iterator ) mutable {
							  handle_connect( obj, err );
						  } );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception starting connect", "NetSocketStream::connect" );
					}
					return *this;
				}

				size_t &NetSocketStream::buffer_size( ) {
					daw::exception::daw_throw_not_implemented( );
				}

				daw::nodepp::lib::net::impl::BoostSocket &NetSocketStream::socket( ) {
					return m_data->m_socket;
				}

				daw::nodepp::lib::net::impl::BoostSocket const &NetSocketStream::socket( ) const {
					return m_data->m_socket;
				}

				NetSocketStream &NetSocketStream::write_async( base::write_buffer buff ) {
					try {
						daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
						                                   "Attempt to use a closed NetSocketStream" );
						auto obj = *this;
						m_data.visit( [&]( ss_data_t &data ) {
							data.m_bytes_written += buff.size( );

							++data.m_pending_writes;
							data.m_socket.write_async( buff.asio_buff( ),
							                           [obj, buff]( base::ErrorCode err, size_t bytes_transfered ) mutable {
								                           handle_write( obj, std::move( buff ), err, bytes_transfered );
							                           } );
						} );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception while writing", "NetSocketStream::write_async" );
					}
					return *this;
				}

				NetSocketStream &NetSocketStream::send_file( daw::string_view file_name ) {
					try {
						daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
						                                   "Attempt to use a closed NetSocketStream" );

						m_data.visit( [&]( ss_data_t &data ) {
							data.m_bytes_written += boost::filesystem::file_size( boost::filesystem::path{file_name.data( )} );
							daw::filesystem::memory_mapped_file_t<char> mmf{file_name};
							daw::exception::daw_throw_on_false( mmf, "Could not open file" );
							boost::asio::const_buffers_1 buff{mmf.data( ), mmf.size( )};
							data.m_socket.write( buff );
						} );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception while writing from file", "NetSocketStream::send_file" );
					}
					return *this;
				}

				NetSocketStream &NetSocketStream::send_file_async( string_view file_name ) {
					try {
						daw::exception::daw_throw_on_true( is_closed( ) || !can_write( ),
						                                   "Attempt to use a closed NetSocketStream" );

						auto mmf = daw::nodepp::impl::make_shared_ptr<daw::filesystem::memory_mapped_file_t<char>>( file_name );
						daw::exception::daw_throw_on_false( mmf, "Could not open file" );
						daw::exception::daw_throw_on_false( *mmf, "Could not open file" );

						auto buff = daw::nodepp::impl::make_shared_ptr<boost::asio::const_buffers_1>( mmf->data( ), mmf->size( ) );
						daw::exception::daw_throw_on_false( buff, "Could not create buffer" );

						m_data.visit( [&]( ss_data_t &data ) {
							++data.m_pending_writes;
							data.m_socket.write_async(
							  *buff, [ obj = *this, buff, mmf ]( base::ErrorCode err, size_t bytes_transfered ) mutable {
								  handle_write( obj, err, bytes_transfered );
							  } );
						} );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception while writing from file",
						            "NetSocketStream::send_file_async" );
					}
					return *this;
				}

				NetSocketStream &NetSocketStream::end( ) {
					try {
						m_data.visit( []( ss_data_t &data ) {
							data.m_state.end( true );
							if( data.m_socket.is_open( ) ) {
								data.m_socket.shutdown( );
							}
						} );
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception calling shutdown on socket", "NetSocketStream::end" );
					}
					return *this;
				}

				void NetSocketStream::close( bool emit_cb ) {
					try {
						m_data.visit( []( ss_data_t &data ) {
							data.m_state.closed( true );
							data.m_state.end( true );
							if( data.m_socket.is_open( ) ) {
								data.m_socket.cancel( );
								data.m_socket.reset_socket( );
							}
						} );
						if( emit_cb ) {
							emit_closed( );
						}
					} catch( ... ) {
						emit_error( std::current_exception( ), "Exception while closing socket", "NetSocketStream::close" );
					}
				}

				void NetSocketStream::cancel( ) {
					m_data->m_socket.cancel( );
				}

				NetSocketStream &NetSocketStream::set_timeout( int32_t ) {
					daw::exception::daw_throw_not_implemented( );
				}

				NetSocketStream &NetSocketStream::set_no_delay( bool ) {
					daw::exception::daw_throw_not_implemented( );
				}

				NetSocketStream &NetSocketStream::set_keep_alive( bool, int32_t ) {
					daw::exception::daw_throw_not_implemented( );
				}

				std::string NetSocketStream::remote_address( ) const {
					return m_data->m_socket.remote_endpoint( ).address( ).to_string( );
				}

				std::string NetSocketStream::local_address( ) const {
					return m_data->m_socket.local_endpoint( ).address( ).to_string( );
				}

				uint16_t NetSocketStream::remote_port( ) const {
					return m_data->m_socket.remote_endpoint( ).port( );
				}

				uint16_t NetSocketStream::local_port( ) const {
					return m_data->m_socket.local_endpoint( ).port( );
				}

				size_t NetSocketStream::bytes_read( ) const {
					return m_data->m_bytes_read;
				}

				size_t NetSocketStream::bytes_written( ) const {
					return m_data->m_bytes_written;
				}

				// StreamReadable Interface
				base::data_t NetSocketStream::read( ) {
					return impl::get_clear_buffer( m_data->m_response_buffers, m_data->m_response_buffers.size( ), 0 );
				}

				base::data_t NetSocketStream::read( size_t ) {
					daw::exception::daw_throw_not_implemented( );
				}

				bool NetSocketStream::is_closed( ) const {
					return m_data->m_state.closed( );
				}

				bool NetSocketStream::is_open( ) const {
					return m_data->m_socket.is_open( );
				}

				bool NetSocketStream::can_write( ) const {
					return !m_data->m_state.end( );
				}

				void NetSocketStream::write( base::data_t const &data ) {
					m_data->m_socket.write( boost::asio::buffer( data ) );
				}

				void NetSocketStream::emit_data_received( std::shared_ptr<base::data_t> buffer, bool end_of_file ) {
					emitter( ).emit( "data_received", std::move( buffer ), end_of_file );
				}

				void NetSocketStream::emit_eof( ) {
					emitter( ).emit( "eof" );
				}

				void NetSocketStream::emit_closed( ) {
					emitter( ).emit( "closed" );
				}

				void set_ipv6_only( boost::asio::ip::tcp::acceptor &acceptor, ip_version ip_ver ) {
					if( ip_ver == ip_version::ipv4_v6 ) {
						acceptor.set_option( boost::asio::ip::v6_only{false} );
					} else if( ip_ver == ip_version::ipv4_v6 ) {
						acceptor.set_option( boost::asio::ip::v6_only{true} );
					}
				}

				NetSocketStream &operator<<( NetSocketStream &socket, daw::string_view message ) {
					daw::exception::daw_throw_on_false( socket, "Attempt to use a null NetSocketStream" );

					socket.write_async( message );
					return socket;
				}


			} // namespace net
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
