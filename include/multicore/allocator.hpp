// Copyright (c) 2024 - present, Yoram Janssen and Multicore contributors
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// --- Optional exception to the license ---
//
// As an exception, if, as a result of your compiling your source code, portions
// of this Software are embedded into a machine-executable object form of such
// source code, you may redistribute such embedded portions in such object form
// without including the above copyright and permission notices.

#ifndef MTC_ALLOCATOR_HPP
#define MTC_ALLOCATOR_HPP

#include "detail/config.hpp"

#include <concepts>

namespace mtc {

  /**
   * \defgroup allocator allocator
   * \brief The `allocator` module provides facilities for working with allocators.
   */

  /// \concept mtc::allocator
  /// \ingroup allocator
  /// \brief Specifies the requirements for a type to be considered an allocator.
  ///
  /// The `mtc::allocator` concept defines the necessary requirements for a type to qualify as an allocator.
  /// An allocator type must provide a `value_type` member type, be copy and move constructible, and be equality comparable.
  /// Additionally, it must implement the `allocate` and `deallocate` methods to manage memory allocation and deallocation.
  ///
  /// \tparam T The type to check for allocator requirements.
  ///
  /// \details
  /// The `mtc::allocator` concept ensures that a type meets the following criteria:
  /// - **Member Type**: The type must have a `value_type` member type.
  /// - **Copy Constructible**: The type must be copy constructible.
  /// - **Move Constructible**: The type must be move constructible.
  /// - **Equality Comparable**: The type must be equality comparable.
  /// - **Allocate Method**: The type must provide an `allocate` method that takes a size and returns a pointer to `value_type`.
  /// - **Deallocate Method**: The type must provide a `deallocate` method that takes a pointer to `value_type` and a size.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// static_assert(mtc::allocator<std::allocator<int>>);
  /// \endcode
  ///
  /// In the above example, `std::allocator<int>` is checked to ensure it satisfies the `allocator` concept.
  ///
  /// \note This concept is part of the `[allocator]` module and is used to enforce allocator requirements in template parameters.
  template <class T>
  concept allocator = requires { typename std::remove_cvref_t<T>::value_type; } &&                                 // Has a `value_type` member type.
                      std::copy_constructible<T> &&                                                                // Copy constructible.
                      std::move_constructible<T> &&                                                                // Move constructible.
                      std::equality_comparable<T> &&                                                               // Equality comparable.
                      requires(T &&a, const T &b, T c, size_t n, typename std::remove_cvref_t<T>::value_type *p) {
                        { MTC_FWD(a).allocate(n) } -> std::same_as<typename std::remove_cvref_t<T>::value_type *>; // `allocate` method
                        { MTC_FWD(a).deallocate(p, n) } -> std::same_as<void>;                                     // `deallocate` method
                      };

} // namespace mtc

#endif // MTC_ALLOCATOR_HPP
