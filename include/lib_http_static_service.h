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

#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>

#include <daw/daw_string_view.h>
#include <daw/json/daw_json_link.h>

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

				using HttpStaticService = std::shared_ptr<impl::HttpStaticServiceImpl>;

				namespace impl {
					class HttpStaticServiceImpl : public daw::nodepp::base::enable_shared<HttpStaticServiceImpl>,
					                              public daw::nodepp::base::StandardEvents<HttpStaticServiceImpl> {
						std::string m_base_path;
						boost::filesystem::path m_local_filesystem_path;
						std::vector<std::string> m_default_filenames;

					public:
						HttpStaticServiceImpl(
						  daw::string_view base_url_path, daw::string_view local_filesystem_path,
						  daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );

						HttpStaticServiceImpl( ) = delete;
						~HttpStaticServiceImpl( ) override;
						HttpStaticServiceImpl( HttpStaticServiceImpl const & ) = default;
						HttpStaticServiceImpl( HttpStaticServiceImpl && ) noexcept = default;
						HttpStaticServiceImpl &operator=( HttpStaticServiceImpl const & ) = default;
						HttpStaticServiceImpl &operator=( HttpStaticServiceImpl && ) noexcept = default;

						HttpStaticServiceImpl &connect( HttpSite const &site );

						std::string const &get_base_path( ) const;
						boost::filesystem::path const &get_local_filesystem_path( ) const;

						std::vector<std::string> &get_default_filenames( );
						std::vector<std::string> const &get_default_filenames( ) const;
					}; // HttpStaticServiceImpl
				}    // namespace impl

				HttpStaticService create_static_service( daw::string_view base_url_path,
				                                         daw::string_view local_filesystem_path );
			} // namespace http
		}   // namespace lib
	}     // namespace nodepp
} // namespace daw
