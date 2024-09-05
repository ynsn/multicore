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

#ifndef MTC_SYSTEM_CONTEXT_HPP
#define MTC_SYSTEM_CONTEXT_HPP

#include "coroutine.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace mtc {
  /*struct dispatch {
    static auto submit(std::coroutine_handle<> handle) noexcept -> uint32_t {
      const auto work = CreateThreadpoolWork(
          +[](PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK work) noexcept {
            if (auto handle = std::coroutine_handle<>::from_address(context)) {
              if (handle.done() == false) handle.resume();
            }
            // CloseThreadpoolWork(work);
          },
          handle.address(), nullptr);

      if (work == nullptr) {
        return GetLastError();
      }

      SubmitThreadpoolWork(work);
      return S_OK;
    }

    constexpr auto await_ready() const noexcept -> bool { return false; }
    auto await_suspend(std::coroutine_handle<> awaited) noexcept -> void { auto work = submit(awaited); }
    constexpr auto await_resume() const noexcept -> void {}
  };*/

  struct system_scheduler {
    struct schedule_operation {
      using scheduler_type = system_scheduler;
      static auto submit(std::coroutine_handle<> handle) noexcept -> uint32_t {
        const auto work = CreateThreadpoolWork(
            +[](PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK work) noexcept {
              if (const auto handle = std::coroutine_handle<>::from_address(context)) {
                if (handle.done() == false) handle.resume();
              }
              CloseThreadpoolWork(work);
            },
            handle.address(), nullptr);

        if (work == nullptr) {
          return GetLastError();
        }

        SubmitThreadpoolWork(work);
        return S_OK;
      }

      constexpr auto await_ready() const noexcept -> bool { return false; }
      auto await_suspend(const std::coroutine_handle<> awaited) noexcept -> std::coroutine_handle<> {
        auto work = submit(awaited);
        return std::noop_coroutine();
      }
      constexpr auto await_resume() const noexcept -> void {}
    };

    auto schedule() const noexcept -> schedule_operation { return {}; }

    constexpr auto operator==(const system_scheduler &) const noexcept -> bool { return true; }
    system_scheduler() noexcept {
      // printf("system_scheduler created\n");
    }
    system_scheduler(const system_scheduler &other) : moved(other.moved) { printf("system_scheduler copied\n"); }
    system_scheduler(system_scheduler &&other) noexcept : moved(other.moved) {
      other.moved = true;
      // printf("system_scheduler moved\n");
    }
    ~system_scheduler() noexcept {
      if (!moved) {
        // printf("system_scheduler destroyed\n");
      }
    }

    auto operator=(const system_scheduler &other) noexcept -> system_scheduler & {
      moved = other.moved;
      // printf("system_scheduler copy assigned\n");
      return *this;
    }

    auto operator=(system_scheduler &&other) noexcept -> system_scheduler & {
      moved = MTC_MOVE(other.moved);
      // printf("system_scheduler move assigned\n");
      return *this;
    }
    bool moved{false};
  };

  class system_context {
   public:
    system_context() = default;
    system_context(const system_context &) = delete;
    system_context(system_context &&) noexcept = delete;
    ~system_context() noexcept = default;

    constexpr auto operator=(const system_context &) -> system_context & = delete;
    constexpr auto operator=(system_context &&) -> system_context & = delete;

    [[nodiscard]] auto get_scheduler() const noexcept -> system_scheduler { return {}; }
  };

}  // namespace mtc

#endif  // MTC_SYSTEM_CONTEXT_HPP
