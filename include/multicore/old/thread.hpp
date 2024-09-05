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

#ifndef MTC_THREAD_HPP
#define MTC_THREAD_HPP

#include "coroutine.hpp"
#include "env.hpp"

#include <atomic>
#include <chrono>

namespace mtc {

  /**
   * \defgroup thread thread
   * \brief The `thread` module provides facilities for working with threads and thread synchronization.
   */

  template <class T>
  concept customized_sync_wait = requires(T&& t) {
    { MTC_FWD(t).sync_wait() };
  };

  /// \ingroup thread
  /// \brief The `futex` class provides a fast userspace mutex that can be used for synchronization between threads or as a building block for
  /// higher level synchronization primitives. The `futex` aims to be the fastest possible synchronization primitive on platforms where it is
  /// supported. Otherwise, it is desired to fall back to a fast custom implementation.
  class futex {
   public:
    futex() noexcept = default;
    explicit futex(const uint8_t val) noexcept : state(val) {}
    futex(futex const&) = delete;
    futex(futex&&) noexcept = delete;
    ~futex() noexcept = default;

    auto operator=(futex const&) -> futex& = delete;
    auto operator=(futex&&) noexcept -> futex& = delete;
    friend auto operator==(const futex& a, const futex& b) noexcept -> bool;
    friend auto operator!=(const futex& a, const futex& b) noexcept -> bool;

   public:
    /// \brief The `wait()` function blocks the current thread until the futex is signaled to wake up.
    auto wait() noexcept -> void;

    /// \brief The `wake_one()` function signals one and only one blocked thread to wake up and proceed.
    auto wake_one() noexcept -> void;

    /// \brief The `wake_all()` function signals all blocked threads to wake up and proceed.
    auto wake_all() noexcept -> void;

    /// \brief The `reset()` function resets and reinitializes the futex.
    auto reset() noexcept -> void;

   private:
    std::atomic_uint8_t state{0};
  };

  namespace this_thread {
    namespace detail {
      template <class Promise, class T>
      struct sync_wait_task_promise_base {
        using promise_type = Promise;
        using result_type = T&&;

        std::remove_reference_t<result_type>* value_pointer;

        [[nodiscard]] constexpr auto yield_value(result_type value_ref) noexcept -> decltype(auto) {
          value_pointer = &value_ref;
          return static_cast<promise_type*>(this)->final_suspend();
        }

        constexpr auto return_void() const noexcept -> void {}

        [[nodiscard]] constexpr auto result() const noexcept -> result_type { return static_cast<result_type>(*value_pointer); }
      };

      template <class Promise>
      struct sync_wait_task_promise_base<Promise, void> {
        using promise_type = Promise;
        using result_type = void;

        constexpr auto return_void() const noexcept -> void {}

        constexpr auto result() const noexcept -> result_type {}
      };

      template <class T>
      struct sync_wait_task : allocator_aware_coro_t {
        struct basic_promise_type : sync_wait_task_promise_base<basic_promise_type, T> {
          struct final_awaiter {
            [[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return false; }
            template <class Promise>
            constexpr auto await_suspend(std::coroutine_handle<Promise> handle) noexcept -> void {
              handle.promise().futex.wake_all();
            }
            constexpr auto await_resume() const noexcept -> void {}
          };

          constexpr auto get_return_object() noexcept -> sync_wait_task {
            return sync_wait_task{std::coroutine_handle<basic_promise_type>::from_promise(*this)};
          }
          constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }
          constexpr auto final_suspend() const noexcept -> final_awaiter { return {}; }
          [[noreturn]] constexpr auto unhandled_exception() const noexcept -> void { terminate(); }

          constexpr auto get_env() const noexcept -> decltype(auto) {
            return env(with(get_scheduler, inline_scheduler{}));
          }

          futex futex;
        };

        constexpr explicit sync_wait_task(std::coroutine_handle<basic_promise_type> h) noexcept : handle(h) {}
        constexpr sync_wait_task(const sync_wait_task&) = delete;
        constexpr sync_wait_task(sync_wait_task&&) noexcept = delete;
        constexpr ~sync_wait_task() noexcept {
          if (handle) {
            handle.destroy();
          }
        }

        constexpr auto resume_and_wait() const noexcept -> void {
          handle.resume();
          handle.promise().futex.wait();
        }

        std::coroutine_handle<basic_promise_type> handle;
      };

      template <awaitable Awaitable>
      [[nodiscard]] auto make_sync_wait_task(Awaitable&& awaitable) noexcept -> sync_wait_task<await_result_t<Awaitable>> {
        if constexpr (std::is_void_v<await_result_t<Awaitable>>) {
          co_await MTC_FWD(awaitable);
        } else {
          co_yield co_await MTC_FWD(awaitable);
        }
      }
    }  // namespace detail

    struct sync_wait_t {
      template <class T>
        requires customized_sync_wait<std::remove_cvref_t<T>> || awaitable<std::remove_cvref_t<T>>
      constexpr auto operator()(T&& t) const -> decltype(auto) {
        if constexpr (customized_sync_wait<std::remove_cvref_t<T>>) {
          return MTC_FWD(t).sync_wait(allocator);
        } else {
          auto task = detail::make_sync_wait_task(MTC_FWD(t));
          task.resume_and_wait();
          return task.handle.promise().result();
        }
      }
    };

    inline constexpr auto sync_wait = sync_wait_t{};

    using thread_id = uint64_t;

    auto id() noexcept -> thread_id;

  }  // namespace this_thread
}  // namespace mtc

#endif  // MTC_THREAD_HPP
