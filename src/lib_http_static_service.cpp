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

#include "lib_http_static_service.h"
#include "base_event_emitter.h"
#include "lib_http_request.h"
#include "lib_http_site.h"

namespace daw {
	namespace nodepp {
		namespace lib {
			namespace http {
				namespace impl {
					class HttpStaticServiceImpl;
				}

				template<typename Handler>
				using HttpStaticService = std::shared_ptr<impl::HttpStaticServiceImpl>;

				namespace impl {
					HttpStaticServiceImpl::HttpStaticServiceImpl(
					    std::string base_url_path, std::string local_filesystem_path,
					    daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) )
					    : daw::nodepp::base::StandardEvents<HttpStaticServiceImpl>{std::move( emitter )}
					    , m_base_path{std::move( base_url_path )}
					    , m_local_filesystem_path{std::move( local_filesystem_path )} {

						daw::exception::daw_throw_on_false( )
					}
				}; // namespace impl
			}      // namespace http

			HttpStaticService create_web_service( std::string base_url_path, std::string local_filesystem_path ) {
				return std::make_shared<impl::HttpStaticServiceImpl>( std::move( base_url_path ),
				                                                      std::move( local_filesystem_path ) );
			}
		} // namespace lib
	}     // namespace nodepp
} // namespace daw
} // namespace daw
