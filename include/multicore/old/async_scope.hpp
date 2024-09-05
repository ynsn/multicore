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

#ifndef MTC_ASYNC_SCOPE_HPP
#define MTC_ASYNC_SCOPE_HPP

#include "coroutine.hpp"

namespace mtc {
  class spawn_future;
  class async_scope;

  struct async_scope_task {
    struct promise_type {
      constexpr auto initial_suspend() const noexcept -> std::suspend_never { return {}; }
      constexpr auto final_suspend() const noexcept -> std::suspend_never { return {}; }
      constexpr auto unhandled_exception() const noexcept -> void {}
      constexpr auto get_return_object() const noexcept -> async_scope_task { return {}; }
      constexpr auto return_void() const noexcept -> void {}
    };
  };

  namespace cpo {
    struct spawn_t final {
      template <class Scope, awaitable Awaitable>
        requires requires(Scope &&scope, Awaitable &&awaitable) { MTC_FWD(scope).spawn(MTC_FWD(awaitable)); }
      constexpr auto operator()(Scope &&scope, Awaitable &&awaitable) const noexcept(noexcept(MTC_FWD(scope).spawn(MTC_FWD(awaitable))))
          -> void {
        MTC_FWD(scope).spawn(MTC_FWD(awaitable));
      }
    };

    struct spawn_future_t final {
      template <class Scope, awaitable Awaitable>
        requires requires(Scope &&scope, Awaitable &&awaitable) { MTC_FWD(scope).spawn_future(MTC_FWD(awaitable)); }
      constexpr auto operator()(Scope &&scope, Awaitable &&awaitable) const noexcept(noexcept(MTC_FWD(scope).spawn_future(MTC_FWD(awaitable))))
          -> decltype(auto) {
        MTC_FWD(scope).spawn_future(MTC_FWD(awaitable));
      }
    };
  }  // namespace cpo

  inline constexpr auto spawn = cpo::spawn_t{};
  inline constexpr auto spawn_future = cpo::spawn_t{};

  class async_scope {
   public:
    async_scope() = default;
    async_scope(const async_scope &) = delete;
    async_scope(async_scope &&) noexcept = delete;

    ~async_scope() noexcept = default;

   public:
    constexpr auto operator=(const async_scope &) -> async_scope & = delete;
    constexpr auto operator=(async_scope &&) -> async_scope & = delete;
    constexpr auto operator==(const async_scope &other) const noexcept -> bool {
      if (this == &other) return true;
      return continuation == other.continuation && refcount.load(std::memory_order_acquire) == other.refcount.load(std::memory_order_acquire);
    }

   public:
    template <awaitable Awaitable>
    auto spawn(Awaitable &&awaitable) noexcept -> void {
      []<class A>(async_scope *scope, A &&a) -> async_scope_task {
        scope->retain_ref();
        co_await MTC_FWD(a);
        scope->release_ref();
      }(this, MTC_FWD(awaitable));
    }

    auto join() noexcept {
      struct awaiter {
        async_scope *scope;
        auto await_ready() const noexcept -> bool { return scope->refcount.load(std::memory_order_acquire) == 0; }
        auto await_suspend(std::coroutine_handle<> continuation) noexcept -> bool {
          scope->continuation = continuation;
          return scope->refcount.fetch_sub(1, std::memory_order_acq_rel) > 1;
        }
        constexpr auto await_resume() noexcept -> void {}
      };

      return awaiter{this};
    }

   private:
    auto retain_ref() noexcept -> void { refcount.fetch_add(1, std::memory_order_relaxed); }

    auto release_ref() noexcept -> void {
      if (refcount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        continuation.resume();
      }
    }

    std::coroutine_handle<> continuation{};
    std::atomic_size_t refcount{1ULL};
  };
}  // namespace mtc

#endif  // MTC_ASYNC_SCOPE_HPP
