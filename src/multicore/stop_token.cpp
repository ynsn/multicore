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

#include "multicore/stop_token.hpp"

#include <atomic>
#include <cassert>

namespace mtc {

  namespace detail {
    struct spinloop {
      spinloop() noexcept = default;

      static constexpr auto yield_threshold = 20UL;
      uint32_t count = 0;

      auto wait() noexcept -> void {
        if (count++ < yield_threshold) {
        } else {
          if (count == 0) count = yield_threshold;
          std::this_thread::yield();
        }
      }
    };
  }  // namespace detail

  //---------------------------------------------------------------------------------------------------------------------------------------------
  // class detail::inplace_stop_callback_base
  //---------------------------------------------------------------------------------------------------------------------------------------------

  auto detail::inplace_stop_callback_base::invoke() noexcept -> void { this->callback(this); }

  auto detail::inplace_stop_callback_base::register_callback() noexcept -> void {
    if (source != nullptr) {
      if (!source->try_add_callback(this)) {
        source = nullptr;
        invoke();
      }
    }
  }

  //---------------------------------------------------------------------------------------------------------------------------------------------
  // class inplace_stop_source
  //---------------------------------------------------------------------------------------------------------------------------------------------

  inline constexpr uint8_t stop_requested_flag = 1U;
  inline constexpr uint8_t locked_flag = 2U;

  inplace_stop_source::~inplace_stop_source() noexcept {
    assert((std::atomic_load_explicit(&state, std::memory_order_relaxed) & locked_flag) == 0U);
    assert(callbacks == nullptr);
  }

  auto inplace_stop_source::get_token() const noexcept -> inplace_stop_token { return inplace_stop_token{this}; }

  auto inplace_stop_source::request_stop() noexcept -> bool {
    if (!try_lock(true)) return false;

    notifying = std::this_thread::get_id();

    while (callbacks != nullptr) {
      auto *callback = callbacks;
      callback->previous_pointer = nullptr;
      callbacks = callback->next;
      if (callbacks != nullptr) callbacks->previous_pointer = &callbacks;

      state.store(stop_requested_flag, std::memory_order_release);
      bool removed_during_callback = false;
      callback->removed = &removed_during_callback;
      callback->invoke();

      if (!removed_during_callback) {
        callback->removed = nullptr;
        callback->completed.store(true, std::memory_order_release);
      }

      (void)lock();
    }

    state.store(stop_requested_flag, std::memory_order_release);
    return true;
  }

  auto inplace_stop_source::stop_requested() const noexcept -> bool {
    return (state.load(std::memory_order_acquire) & stop_requested_flag) != 0U;
  }

  auto inplace_stop_source::lock() const noexcept -> uint8_t {
    detail::spinloop spin;
    auto old_state = state.load(std::memory_order_relaxed);
    do {
      while ((old_state & locked_flag) != 0U) {
        spin.wait();
        old_state = state.load(std::memory_order_relaxed);
      }
    } while (!state.compare_exchange_weak(old_state, old_state | locked_flag, std::memory_order_acquire, std::memory_order_relaxed));
    return old_state;
  }

  auto inplace_stop_source::unlock(uint8_t old_state) const noexcept -> void { state.store(old_state, std::memory_order_release); }

  auto inplace_stop_source::try_lock(const bool stop_requested) const noexcept -> bool {
    detail::spinloop spin;
    auto old_state = state.load(std::memory_order_relaxed);
    do {
      while (true) {
        if ((old_state & stop_requested_flag) != 0U) return false;
        if (old_state == 0) break;

        spin.wait();
        old_state = state.load(std::memory_order_relaxed);
      }
    } while (!state.compare_exchange_weak(old_state, stop_requested ? (locked_flag | stop_requested_flag) : locked_flag,
                                          std::memory_order_acq_rel, std::memory_order_relaxed));
    return true;
  }

  auto inplace_stop_source::try_add_callback(detail::inplace_stop_callback_base *callback_base) const noexcept -> bool {
    if (!try_lock(false)) return false;

    callback_base->next = callbacks;
    callback_base->previous_pointer = &callbacks;
    if (callbacks != nullptr) callbacks->previous_pointer = &callback_base->next;
    callbacks = callback_base;
    unlock(0);
    return true;
  }

  auto inplace_stop_source::remove_callback(const detail::inplace_stop_callback_base *callback_base) const noexcept -> void {
    const auto old_state = lock();
    if (callback_base->previous_pointer != nullptr) {
      *callback_base->previous_pointer = callback_base->next;
      if (callback_base->next != nullptr) callback_base->next->previous_pointer = callback_base->previous_pointer;
      unlock(old_state);
    } else {
      const auto notifying_thread = notifying;
      unlock(old_state);
      if (std::this_thread::get_id() == notifying_thread) {
        if (callback_base->removed != nullptr) *callback_base->removed = true;
      } else {
        detail::spinloop spin;
        while (!callback_base->completed.load(std::memory_order_acquire)) spin.wait();
      }
    }
  }

  //---------------------------------------------------------------------------------------------------------------------------------------------
  // class inplace_stop_token
  //---------------------------------------------------------------------------------------------------------------------------------------------

  inplace_stop_token::inplace_stop_token(inplace_stop_token &&other) noexcept : source{other.source} { other.source = nullptr; }

  auto inplace_stop_token::operator=(inplace_stop_token &&other) noexcept -> inplace_stop_token & {
    if (this != &other) {
      source = other.source;
      other.source = nullptr;
    }

    return *this;
  }

  auto inplace_stop_token::stop_requested() const noexcept -> bool { return source != nullptr && source->stop_requested(); }
  auto inplace_stop_token::stop_possible() const noexcept -> bool { return (source != nullptr) && (!source->stop_requested()); }
  auto inplace_stop_token::swap(inplace_stop_token &other) noexcept -> void { std::swap(source, other.source); }
  inplace_stop_token::inplace_stop_token(const inplace_stop_source *src) noexcept : source{src} {}
}  // namespace mtc
