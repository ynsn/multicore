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

#ifndef MTC_TASK_HPP
#define MTC_TASK_HPP

#include "allocator.hpp"
// #include "coroutine.hpp"
#include "env.hpp"

#include "stop_token.hpp"

#include <iostream>
#include <optional>
#include <source_location>

namespace mtc {

  /**
   * \defgroup task task
   * \brief The `task` module provides facilities for working with tasks and task handles.
   */

  /// \ingroup task
  /// \brief The `mtc::scheduler_affinity` enum class is used to specify the affinity of schedulers for tasks.
  enum class scheduler_affinity { none, fixed };

  template <class T>
  struct promise_storage {
    using value_type = T;

    union value_storage {
      value_storage() noexcept = default;
      ~value_storage() noexcept {}

      intptr_t intptr{-1};
      value_type value;
    } storage;

    template <class U>
      requires std::constructible_from<T, U &&>
    constexpr auto return_value(U &&value) noexcept(noexcept(new(mtc::addressof(storage)) T(MTC_FWD(value)))) -> void {
      new (mtc::addressof(storage.value)) T(MTC_FWD(value));
    }

    [[nodiscard]] constexpr auto result() & noexcept -> T & { return storage.value; }
    [[nodiscard]] constexpr auto result() const & noexcept -> const T & { return storage.value; }
    [[nodiscard]] constexpr auto result() && noexcept -> T && { return static_cast<T &&>(storage.value); }
    [[nodiscard]] constexpr auto result() const && noexcept -> const T && { return static_cast<const T &&>(storage.value); }
  };

  template <>
  struct promise_storage<void> {
    constexpr auto return_void() const noexcept -> void {}
    constexpr auto result() const noexcept -> void {}
  };

  template <class Task, class Result, class Context>
  class basic_task_promise : public promise_storage<Result> {
   public:
    using task_type = Task;
    using result_type = Result;
    using handle_type = std::coroutine_handle<basic_task_promise>;

    struct final_awaiter : std::suspend_always {
      template <class Promise>
      constexpr auto await_suspend(std::coroutine_handle<Promise> handle) const noexcept -> std::coroutine_handle<> {
        return handle.promise().continuation().handle();
      }
    };

    constexpr basic_task_promise() noexcept = default;
    constexpr ~basic_task_promise() noexcept = default;

    [[nodiscard]] constexpr auto get_return_object() noexcept -> task_type { return task_type{handle_type::from_promise(*this)}; }
    [[nodiscard]] constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }
    [[nodiscard]] constexpr auto final_suspend() const noexcept -> final_awaiter { return {}; }
    [[noreturn]] auto unhandled_exception() const noexcept -> void {
      const auto ex = std::current_exception();
      try {
        std::rethrow_exception(ex);
      } catch (const std::exception &e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Unhandled exception" << std::endl;
      }

      std::terminate();
    }
    [[nodiscard]] constexpr auto unhandled_stopped() const noexcept -> std::coroutine_handle<> { return co_handle.unhandled_stopped(); }

    template <class Promise>
    constexpr auto set_continuation(std::coroutine_handle<Promise> continuation) noexcept -> void {
      static_assert(!std::same_as<void, Promise>, "Cannot set a continuation on a void promise");
      co_handle = continuation;
    }
    constexpr auto set_continuation(continuation_handle<> handle) noexcept -> void { co_handle = handle; }
    [[nodiscard]] constexpr auto continuation() const noexcept -> continuation_handle<> { return co_handle; }

   public:
    continuation_handle<> co_handle{};
  };

  template <class T, class Context>
  class [[nodiscard]] basic_task {
   public:
    using allocator_aware_coro_concept = allocator_aware_coro_t;
    using result_type = T;
    using basic_promise_type = basic_task_promise<basic_task, T, Context>;
    using handle_type = std::coroutine_handle<basic_promise_type>;

    constexpr basic_task() noexcept : co_handle(nullptr) {}
    constexpr explicit basic_task(std::nullptr_t) noexcept : co_handle(nullptr) {}
    constexpr explicit basic_task(handle_type handle) noexcept : co_handle(handle) {}

    constexpr basic_task(const basic_task &) = delete;
    constexpr basic_task(basic_task &&other) noexcept : co_handle(mtc::exchange(other.co_handle, {})) {}

    constexpr ~basic_task() noexcept { destroy(); }

    constexpr auto operator=(const basic_task &) noexcept -> basic_task & = delete;
    constexpr auto operator=(basic_task &&other) noexcept -> basic_task &;

    constexpr auto destroy() noexcept -> void {
      if (co_handle) mtc::exchange(co_handle, {}).destroy();
    }
    [[nodiscard]] constexpr auto ready() const noexcept -> bool { return (co_handle && co_handle.done()); }

    [[nodiscard]] constexpr auto when_ready() const noexcept -> int;

   private:
    struct awaitable_base {
      handle_type handle;

      constexpr ~awaitable_base() noexcept {
        if (handle) handle.destroy();
      }

      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return !handle || handle.done(); }

      template <class Parent>
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<Parent> parent) -> std::coroutine_handle<> {
        handle.promise().set_continuation(parent);
        return handle;
      }
    };

    template <class U>
    struct awaitable : awaitable_base {
      constexpr auto await_resume() noexcept -> decltype(auto) {
        scope_guard g{[&] { mtc::exchange(awaitable_base::handle, {}).destroy(); }};
        return MTC_MOVE(awaitable_base::handle.promise()).result();
      }
    };

    template <>
    struct awaitable<void> : awaitable_base {
      constexpr auto await_resume() noexcept -> void {
        scope_guard g{[&] { mtc::exchange(awaitable_base::handle, {}).destroy(); }};
      }
    };

   public:
    friend auto operator co_await(basic_task &&t) noexcept -> awaitable<T> { return awaitable<T>{mtc::exchange(t.co_handle, {})}; }
    friend auto operator co_await(basic_task &t) noexcept -> awaitable<T> = delete;

   private:
    handle_type co_handle{};
  };

  template <class Context, awaitable Awaitable>
  [[nodiscard]] auto make_task(Awaitable &&awaitable) noexcept -> basic_task<await_result_t<Awaitable>, Context> {
    co_return co_await MTC_FWD(awaitable);
  }

  namespace this_task {
    struct current_scheduler_t {
      any_scheduler2 *scheduler{};
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return false; }

      template <class Promise>
      constexpr auto await_suspend(std::coroutine_handle<Promise> handle) noexcept -> std::coroutine_handle<> {
        decltype(auto) sched = get_scheduler(get_env(handle.promise()));
        scheduler = &sched;
        return handle;
      }

      [[nodiscard]] auto await_resume() const noexcept -> any_scheduler2 * { return scheduler; }
    };

    [[nodiscard]] inline auto scheduler() noexcept -> current_scheduler_t { return {current_scheduler_t{}}; }

  }  // namespace this_task

  template <class T, class Context>
  constexpr auto basic_task<T, Context>::operator=(basic_task &&other) noexcept -> basic_task & {
    if (*this != other) {
      if (co_handle) co_handle.destroy();
      co_handle = mtc::exchange(other.co_handle, {});
    }

    return *this;
  }

  template <class T, class Context = void>
  using task = basic_task<T, Context>;

}  // namespace mtc

#endif  // MTC_TASK_HPP
