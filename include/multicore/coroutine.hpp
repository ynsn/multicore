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

#include "detail/config.hpp"

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
  /// \brief Concept to check if a type satisfies the requirements of a C++20 awaiter.
  ///
  /// The `awaiter` concept is used to determine if a given type `Awaiter` implements the necessary functions
  /// to be considered a valid C++20 awaiter. An awaiter is a type that can be used with the `co_await` operator
  /// in coroutines, providing mechanisms to suspend and resume execution.
  ///
  /// \tparam Awaiter The type to be checked.
  /// \tparam Promise The promise type associated with the coroutine, defaulting to `void`.
  ///
  /// \details
  /// The `awaiter` concept requires the type `Awaiter` to implement the following member functions:
  /// - `await_ready()`: This function should return a value that is contextually convertible to `bool`.
  /// - `await_suspend(std::coroutine_handle<Promise>)`: This function should return a type that satisfies the
  ///   `detail::await_suspend_result` concept, which includes `void`, `bool`, or `std::coroutine_handle<Promise>`.
  /// - `await_resume()`: This function should return the result of the awaited operation.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// struct MyAwaiter {
  ///     bool await_ready() { return false; }
  ///     void await_suspend(std::coroutine_handle<>) {}
  ///     void await_resume() {}
  /// };
  ///
  /// static_assert(mtc::awaiter<MyAwaiter>, "MyAwaiter does not satisfy the awaiter concept");
  /// \endcode
  ///
  /// In the above example, `MyAwaiter` is a type that satisfies the `awaiter` concept, as it implements the required
  /// member functions.
  ///
  /// \note This concept is part of the `mtc::coroutine` module and is essential for defining custom awaiters
  ///       that can be used with C++20 coroutines.
  template <class Awaiter, class Promise = void>
  concept awaiter = requires(Awaiter &a, std::coroutine_handle<Promise> handle) {
    { a.await_ready() ? void() : void() };
    { a.await_suspend(handle) } -> detail::await_suspend_result;
    { a.await_resume() };
  };

  /// \ingroup coroutine
  /// \brief The `mtc::awaiter_of` concept is satisfied if and only if type `Awaiter` implements the necessary functions for a valid C++20
  /// awaiter and the return type of `await_resume` is convertible to `Result`.
  /// \details An awaiter is a type that implements the necessary functions for a valid C++20 awaiter. An awaiter must implement the following
  /// member functions:
  ///  - \code auto await_ready(); \endcode `The return type of this function must be contextually convertible to bool.`
  ///  - \code auto await_suspend(std::coroutine_handle<void>); \endcode `The return type of this function must be a valid await_suspend result,
  ///   which is either void, bool, or std::coroutine_handle<Promise>.`
  ///  - \code auto await_resume(); \endcode `The return type of this function must be the same or convertible to type Result.`
  ///
  /// Furthermore, this concept mandates that these member functions are callable on a lvalue of type `Awaiter`.
  /// \tparam Awaiter The type to check.
  /// \tparam Result The result type of the awaiter.
  /// \tparam Promise The promise type associated with the awaiter.
  /// \see https://en.cppreference.com/w/cpp/language/coroutines
  template <class Awaiter, class Result, class Promise = void>
  concept awaiter_of = awaiter<Awaiter, Promise> && requires(Awaiter &a) {
    { a.await_resume() } -> std::convertible_to<Result>;
  };

  template <class Awaitable>
  static constexpr auto get_awaiter(Awaitable &&awaitable, void *promise = nullptr) {
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

  /// \ingroup coroutine
  /// \brief The `mtc::awaitable` concept is satisfied if and only if type `Awaitable` is a type where `mtc::get_awaiter` can be called on and
  /// returns a type that satisfies the `mtc::awaiter` concept.
  template <class Awaitable, class Promise = void>
  concept awaitable = requires(Awaitable &&a, Promise *p) {
    { get_awaiter(MTC_FWD(a), p) } -> awaiter<Promise>;
  };

  template <class Awaitable, class Result, class Promise = void>
  concept awaitable_of = requires(Awaitable &&a, Promise *p) {
    { get_awaiter(MTC_FWD(a), p) } -> awaiter_of<Result>;
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
