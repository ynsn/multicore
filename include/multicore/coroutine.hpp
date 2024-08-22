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

#include "allocator.hpp"
#include "utility.hpp"

#include <coroutine>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

namespace mtc {

  /**
   * \defgroup coroutine coroutine
   * \brief The `coroutine` module provides utilities for working with C++20 coroutines.
   * \todo Add more documentation.
   */

  struct default_coroutine_allocator {
    using value_type = char;

    friend constexpr auto operator==(default_coroutine_allocator const &, default_coroutine_allocator const &) noexcept -> bool { return true; }

    [[nodiscard]] static auto allocate(const size_t size) -> char * {
      return static_cast<char *>(malloc(size));
    }

    static auto deallocate(char *pointer, const size_t size) noexcept -> void {
      free(pointer);
    }
  };

  namespace detail {
    template <class T>
    inline constexpr auto is_coroutine_handle_v = false;
    template <class Promise>
    inline constexpr auto is_coroutine_handle_v<std::coroutine_handle<Promise>> = true;
    template <class T>
    concept await_suspend_result = std::same_as<T, void> || std::same_as<T, bool> || is_coroutine_handle_v<T>;
  }  // namespace detail

  /// \ingroup coroutine
  /// \brief The `mtc::awaiter` concept is satisfied if and only if type `T` is a valid awaiter as defined by the C++20 standard.
  /// \tparam T The type to check. \tparam Promise The promise type associated with the awaiter.
  /// \tparam Promise The promise type associated with the awaiter.
  /// \see https://en.cppreference.com/w/cpp/language/coroutines
  template <class T, class Promise = void>
  concept awaiter = requires(T &&a, std::coroutine_handle<Promise> handle) {
    { a.await_ready() ? 1 : 0 };
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
  concept awaiter_of = awaiter<T, Promise> && requires(T &&a) {
    { a.await_resume() } -> std::same_as<Result>;
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

  template <class Awaitable>
    requires awaitable<Awaitable>
  using awaiter_type_t = decltype(get_awaiter(declval<Awaitable>(), static_cast<void *>(nullptr)));

  template <class Awaitable, class Promise = void>
    requires awaitable<Awaitable, Promise>
  using await_result_t = decltype(as_lvalue(get_awaiter(declval<Awaitable>(), static_cast<Promise *>(nullptr))).await_resume());

  template <class Promise = void>
  class continuation_handle;

  template <>
  class continuation_handle<void> {
   public:
    constexpr continuation_handle() noexcept = default;
    template <class Promise>
    constexpr continuation_handle(std::coroutine_handle<Promise> handle) noexcept : coro(handle) {
      if constexpr (requires(Promise *promise) { promise.unhandled_stopped(); }) {
        on_stopped = [](void *address) noexcept -> std::coroutine_handle<> {
          return std::coroutine_handle<Promise>::from_address(address).promise().unhandled_stopped();
        };
      }
    }

    [[nodiscard]] constexpr auto handle() const noexcept -> std::coroutine_handle<> { return coro; }
    [[nodiscard]] constexpr auto unhandled_stopped() const noexcept -> std::coroutine_handle<> { return on_stopped(coro.address()); }

   private:
    using stopped_callback = std::coroutine_handle<> (*)(void *) noexcept;

    [[noreturn]] static auto default_stopped_callback(void *) noexcept -> std::coroutine_handle<> { terminate(); }

    std::coroutine_handle<> coro{};
    stopped_callback on_stopped{default_stopped_callback};
  };

  template <class Promise>
  class continuation_handle {
   public:
    constexpr continuation_handle() noexcept = default;
    constexpr continuation_handle(std::coroutine_handle<Promise> handle) noexcept : hnd(handle) {}

    [[nodiscard]] constexpr auto handle() const noexcept -> std::coroutine_handle<Promise> {
      return std::coroutine_handle<Promise>::from_address(hnd.handle().address());
    }
    [[nodiscard]] constexpr auto unhandled_stopped() const noexcept -> std::coroutine_handle<> { return hnd.unhandled_stopped(); }

   private:
    continuation_handle<> hnd;
  };

  struct allocator_aware_coro_t {
    using allocator_aware_coro_concept = allocator_aware_coro_t;
  };

  /// \defgroup allocator_aware Allocator Aware Coroutines
  /// \ingroup coroutine
  /// \brief multicore provides various helpers and utilities useful when writing or using custom coroutines. This includes the
  ///      `allocator_aware_coro` concept, which is used to define custom coroutines that are aware of allocators.

  /// \ingroup allocator_aware
  /// \brief Wow
  template <class T>
  concept allocator_aware_coro = requires { typename T::allocator_aware_coro_concept; } &&                               //
                                 std::derived_from<typename T::allocator_aware_coro_concept, allocator_aware_coro_t> &&  //
                                 requires { typename T::basic_promise_type; };                                           //

  template <class T>
  concept coro_allocator = allocator_for<T, char>;

  template <class Promise, class Coroutine, class Allocator>
  struct allocator_aware_coro_promise : public Promise {
    using basic_promise_type = Promise;
    using coroutine_type = Coroutine;
    using allocator_type = Allocator;

    struct allocator_info {
      size_t size;
    };

    template <size_t alignment, typename T>
    static constexpr T align(T num) {
      return num + (alignment - 1) & ~(alignment - 1);
    }

    static constexpr auto allocator_offset = align<alignof(Allocator)>(sizeof(allocator_info));
    static constexpr auto memory_offset = align<alignof(std::max_align_t)>(allocator_offset + sizeof(Allocator));

    template <class... Args>
    constexpr auto operator new(size_t size, Args &&...args) noexcept(false) -> void * {
      auto allocator = mtc::get_value_with_in<with_allocator>(MTC_FWD(args)...).allocator;

      if constexpr (!std::is_empty_v<allocator_type>) {
        auto *memory_ptr = mtc::allocate(allocator, size + memory_offset);
        new (reinterpret_cast<void *>(static_cast<char *>(memory_ptr))) allocator_info{size + memory_offset};
        new (reinterpret_cast<void *>(static_cast<char *>(memory_ptr) + allocator_offset)) allocator_type{allocator};

        return reinterpret_cast<void *>(static_cast<char *>(memory_ptr) + memory_offset);
      } else {
        auto *memory_ptr = mtc::allocate(allocator, size);
        return reinterpret_cast<void *>(memory_ptr);
      }
    }

    constexpr auto operator delete(void *pointer, size_t s) noexcept -> void {
      if constexpr (!std::is_empty_v<Allocator>) {
        auto *memory_ptr = reinterpret_cast<void *>(static_cast<char *>(pointer) - memory_offset);  // Base pointer
        auto *allocator_info_ptr = std::launder(reinterpret_cast<allocator_info *>(memory_ptr));    // allocation_info pointer
        auto *allocator_ptr = std::launder(reinterpret_cast<Allocator *>((char *)memory_ptr + allocator_offset));
        const auto size = allocator_info_ptr->size;

        Allocator temp_allocator = Allocator{*allocator_ptr};
        (*allocator_info_ptr).~allocator_info();
        (*allocator_ptr).~Allocator();

        mtc::deallocate(temp_allocator, static_cast<char *>(memory_ptr), size);
        temp_allocator.~Allocator();
      } else {
        auto *memory_ptr = reinterpret_cast<void *>(static_cast<char *>(pointer));  // Base pointer
        Allocator temp_allocator = Allocator{};
        mtc::deallocate(temp_allocator, static_cast<char *>(memory_ptr), s);
        temp_allocator.~Allocator();
      }
    }
  };
}  // namespace mtc

namespace std {
  template <class T, class... Args>
    requires mtc::allocator_aware_coro<T> && !std::same_as<void, decltype(mtc::get_value_with_in<mtc::with_allocator>(declval<Args>()...))>
  struct coroutine_traits<T, Args...> {
    using promise_type = mtc::allocator_aware_coro_promise<typename T::basic_promise_type, T, decltype(mtc::get_value_with_in<mtc::with_allocator>(declval<Args>()...).allocator)>;
  };

  template <class T, class... Args>
    requires mtc::allocator_aware_coro<T> //&& !mtc::using_allocator<Args...>
                                           struct coroutine_traits<T, Args...> {
    using promise_type = typename T::basic_promise_type;
  };
}  // namespace std

#endif  // MTC_COROUTINE_HPP
