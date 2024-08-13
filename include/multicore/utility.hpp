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

#ifndef MTC_UTILITY_HPP
#define MTC_UTILITY_HPP

namespace mtc {

  namespace detail {
    template <class T>
    struct remove_reference {
      using type = T;
    };

    template <class T>
    struct remove_reference<T &> {
      using type = T;
    };

    template <class T>
    struct remove_reference<T &&> {
      using type = T;
    };

    template<class>
    inline constexpr auto always_false = false;
  }  // namespace detail

  template <class T>
  auto as_lvalue(T &&) -> T &;

  template <class T>
  auto declval() noexcept -> T && {
    static_assert(detail::always_false<T>, "Calling declval is ill-formed");
  }

}  // namespace mtc

#define MTC_FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)
#define MTC_FWD_T(t, ...) static_cast<t &&>(__VA_ARGS__)
#define MTC_MOVE(...) static_cast<typename mtc::detail::remove_reference<decltype(__VA_ARGS__)>::type &&>(__VA_ARGS__)

#endif  // MTC_UTILITY_HPP
