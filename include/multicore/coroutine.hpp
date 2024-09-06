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

#ifndef MTC_COROUTINE_HPP
#define MTC_COROUTINE_HPP

#include "config.hpp"

#include <coroutine>
#include <utility>

namespace mtc {

  /**
   * \defgroup coroutine coroutine
   * \brief The `coroutine` module provides utilities for working with C++20 coroutines.
   * \todo Add more documentation.
   */

  namespace detail {
    template <class T>
    struct valid_await_suspend_result_type {
      static constexpr auto value = false;
    };
    template <>
    struct valid_await_suspend_result_type<void> {
      static constexpr auto value = true;
    };
    template <>
    struct valid_await_suspend_result_type<bool> {
      static constexpr auto value = true;
    };
    template <class Promise>
    struct valid_await_suspend_result_type<std::coroutine_handle<Promise>> {
      static constexpr auto value = true;
    };

    template <class T>
    concept await_suspend_result = valid_await_suspend_result_type<T>::value;
  }  // namespace detail

  /// \ingroup coroutine
  /// \brief The `mtc::awaiter` concept is satisfied if and only if type `T` is a valid awaiter as defined by the C++20 standard.
  /// \tparam Awaiter The type to check.
  /// \tparam Promise The promise type associated with the awaiter.
  /// \see https://en.cppreference.com/w/cpp/language/coroutines
  template <class Awaiter, class Promise = void>
  concept awaiter = requires(Awaiter &a, std::coroutine_handle<Promise> handle) {
    { a.await_ready() ? void() : void() };
    { a.await_suspend(handle) } -> detail::await_suspend_result;
    { a.await_resume() };
  };

  /// \ingroup coroutine
  /// \brief The `mtc::awaiter_of` concept is satisfied if and only if type `T` is a valid `mtc::awaiter` that returns the given result
  /// type.
  /// \tparam T The type to check.
  /// \tparam Result The result type to check for.
  /// \tparam Promise The promise type associated with the awaiter.
  /// \see https://en.cppreference.com/w/cpp/language/coroutines
  template <class T, class Result, class Promise = void>
  concept awaiter_of = awaiter<T, Promise> && requires(T &a) {
    { a.await_resume() } -> std::convertible_to<Result>;
  };

  template <class Awaitable>
  static constexpr awaiter decltype(auto) get_awaiter(Awaitable &&awaitable, void * = nullptr) {
    if constexpr (requires { MTC_FWD(awaitable).operator co_await(); }) {
      return MTC_FWD(awaitable).operator co_await();
    } else if constexpr (requires { operator co_await(MTC_FWD(awaitable)); }) {
      return operator co_await(MTC_FWD(awaitable));
    } else {
      return MTC_FWD(awaitable);
    }
  }

  template <class Awaitable, class Promise>
  static constexpr awaiter decltype(auto) get_awaiter(Awaitable &&awaitable, Promise *promise)
    requires requires { promise->await_transform(MTC_FWD(awaitable)); }
  {
    if constexpr (requires { promise->await_transform(MTC_FWD(awaitable)).operator co_await(); }) {
      return promise->await_transform(MTC_FWD(awaitable)).operator co_await();
    } else if constexpr (requires { operator co_await(promise->await_transform(MTC_FWD(awaitable))); }) {
      return operator co_await(promise->await_transform(MTC_FWD(awaitable)));
    } else {
      return promise->await_transform(MTC_FWD(awaitable));
    }
  }

  template <class Awaitable, class Promise = void>
  concept awaitable = requires(Awaitable &&awaitable, Promise *promise) {
    { get_awaiter(MTC_FWD(awaitable), promise) } -> awaiter<Promise>;
  };

  template <class Awaitable, class Result, class Promise = void>
  concept awaitable_of = requires(Awaitable &&awaitable, Promise *promise) {
    { get_awaiter(MTC_FWD(awaitable), promise) } -> awaiter_of<Result>;
  };

  template <class T, class Promise = void>
  struct awaiter_type {
    using type = decltype(get_awaiter(std::declval<T>(), static_cast<Promise *>(nullptr)));
  };

  template <class Awaitable, class Promise = void>
  using awaiter_type_t = typename awaiter_type<Awaitable, Promise>::type;

  template <class T, class Promise = void>
  struct await_result {};

  template <awaitable Awaitable, class Promise>
  struct await_result<Awaitable, Promise> {
    using type = decltype(std::declval<awaiter_type_t<Awaitable, Promise> &>().await_resume());
  };

  template <class Awaitable, class Promise = void>
  using await_result_t = typename await_result<Awaitable, Promise>::type;

}  // namespace mtc

#endif  // MTC_COROUTINE_HPP
