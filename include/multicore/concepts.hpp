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


#include <concepts>
#include <type_traits>

namespace mtc {

  /**
   * \defgroup concepts concepts
   * \brief The `concepts` module provides C++20 concepts comparable to the ones in the standard library.
   */

  namespace detail {
    template <template <class> class>
    struct template_alias {};

    template <template <class...> class T, class U>
    inline constexpr auto instance_of_impl = false;
    template <template <class...> class T, class... Args>
    inline constexpr auto instance_of_impl<T, T<Args...>> = true;
  }  // namespace detail

  template<class T, template<class...> class Of>
  concept instance_of = detail::instance_of_impl<Of, T>;

  template <class Fn, class... Args>
  concept callable = requires(Fn &&fn, Args &&...args) { static_cast<Fn &&>(fn)(static_cast<Args &&>(args)...); };

  template <class Fn, class... Args>
  concept nothrow_callable = callable<Fn, Args...> && requires(Fn &&fn, Args &&...args) {
    { static_cast<Fn &&>(fn)(static_cast<Args &&>(args)...) } noexcept;
  };

  template <class T>
  concept boolean_testable = std::convertible_to<T, bool> && requires(T &&t) {
    { !static_cast<T &&>(t) } -> std::convertible_to<bool>;
  };
}  // namespace mtc

#endif  // MTC_CONCEPTS_HPP
