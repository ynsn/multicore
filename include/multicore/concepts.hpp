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

#ifndef MTC_CONCEPTS_HPP
#define MTC_CONCEPTS_HPP

namespace mtc {

  /**
   * \defgroup concepts concepts
   * \brief The `concepts` module provides C++20 concepts comparable to the ones in the standard library.
   */

  namespace detail {
    template <class T, class U>
    inline constexpr auto is_same_v = false;
    template <class T>
    inline constexpr auto is_same_v<T, T> = true;
  }  // namespace detail

  /// \ingroup concepts
  /// \brief The `same_as` concept is satisfied if and only if type `A` is the same type as type `B`.
  /// \tparam A The first type.
  /// \tparam B The second type.
  template <class A, class B>
  concept same_as = detail::is_same_v<A, B>;

}  // namespace mtc

#endif  // MTC_CONCEPTS_HPP
