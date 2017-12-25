// The MIT License (MIT)
//
// Copyright (c) 2017 Darrell Wright
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

#include <memory>

namespace daw {
	namespace nodepp {
		namespace impl {
			/// @brief Make a shared_ptr.  This is used so that weak_ptr's are not preventing the reuse of memory for object,
			/// only control block
			/// @tparam T type of value to create
			/// @tparam Args Argument types of T's constructor
			/// @param args Arguments for construction of T
			/// @return a std::shared_ptr<T>
			template<typename T, typename... Args>
			std::shared_ptr<T> make_shared_ptr( Args &&... args ) noexcept( noexcept( std::shared_ptr<T>{
			  new T(std::forward<Args>( args )...)} ) ) {

			  return std::shared_ptr<T>{new T(std::forward<Args>( args )...)};
			}
		} // namespace impl
	}   // namespace nodepp
} // namespace daw
