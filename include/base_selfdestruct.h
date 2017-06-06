#pragma once

#include "base_event_emitter.h"
#include <boost/utility/string_view.hpp>
#include <list>
#include <mutex>

namespace daw {
	namespace nodepp {
		namespace base {
			// Creates a class that will destruct after the event name passed to it is called(e.g. close/end) unless it
			// is referenced elsewhere
			template<typename Derived>
			class SelfDestructing : public daw::nodepp::base::enable_shared<Derived>,
			                        public daw::nodepp::base::StandardEvents<Derived> {

				static std::list<std::shared_ptr<SelfDestructing<Derived>>> &s_selfs( ) noexcept {
					static std::list<std::shared_ptr<SelfDestructing<Derived>>> result;
					return result;
				}

				static std::mutex &s_mutex( ) noexcept {
					static std::mutex result;
					return result;
				}

			  public:
				SelfDestructing( ) = delete;

				explicit SelfDestructing( daw::nodepp::base::EventEmitter emitter )
				    : daw::nodepp::base::StandardEvents<Derived>{std::move( emitter )} {}

				virtual ~SelfDestructing( ) = default;

				SelfDestructing( SelfDestructing const & ) = default;

				SelfDestructing( SelfDestructing && ) = default;

				SelfDestructing &operator=( SelfDestructing const & ) = default;

				SelfDestructing &operator=( SelfDestructing && ) = default;

				void arm( boost::string_view event ) {
					std::unique_lock<std::mutex> lock1( s_mutex( ) );
					auto obj = this->get_ptr( );
					auto pos = s_selfs( ).insert( s_selfs( ).end( ), obj );
					this->emitter( )->add_listener( event.to_string( ) + "_selfdestruct",
					                                [pos]( ) {
						                                std::unique_lock<std::mutex> lock2( s_mutex( ) );
						                                s_selfs( ).erase( pos );
					                                },
					                                true );
				}
			}; // class SelfDestructing
		}      // namespace base
	}          // namespace nodepp
} // namespace daw
