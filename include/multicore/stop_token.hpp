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

#ifndef MTC_STOP_TOKEN_HPP
#define MTC_STOP_TOKEN_HPP

#include "concepts.hpp"

#include <thread>

namespace mtc {
  /**
   * \defgroup stop_token stop_token
   * \brief The `stop_token` module provides facilities for working with stop tokens.
   */

  class inplace_stop_token;
  class inplace_stop_source;

  namespace detail {
    struct inplace_stop_callback_base {
      using callback_type = void(inplace_stop_callback_base *) noexcept;

      inplace_stop_callback_base(const inplace_stop_source *src, callback_type *cb) noexcept : source{src}, callback{cb} {}

      auto invoke() noexcept -> void;
      auto register_callback() noexcept -> void;

      const inplace_stop_source *source;
      callback_type *callback;
      inplace_stop_callback_base *next{nullptr};
      inplace_stop_callback_base **previous_pointer{nullptr};
      bool *removed{nullptr};
      std::atomic<bool> completed{false};
    };
  }  // namespace detail

  template <class Fn>
  class inplace_stop_callback;

  /// \ingroup stop_token
  /// \brief The `mtc::inplace_stop_source` class provides the means to issue stop requests to associated stop tokens.
  /// Contrary to `std::stop_source`, this class guarantees that it does not dynamically allocate memory on the heap.
  class inplace_stop_source {
   public:
    inplace_stop_source() noexcept = default;
    inplace_stop_source(const inplace_stop_source &) = delete;
    inplace_stop_source(inplace_stop_source &&) = delete;
    ~inplace_stop_source() noexcept;

   public:
    auto operator=(const inplace_stop_source &) -> inplace_stop_source & = delete;
    auto operator=(inplace_stop_source &&) -> inplace_stop_source & = delete;

   public:
    /// \brief The `get_token()` method returns a new stop token associated with this stop source.
    /// \return A new stop token.
    [[nodiscard]] auto get_token() const noexcept -> inplace_stop_token;

    /// \brief The `request_stop()` method issues a stop request to all associated stop tokens.
    /// \return `true` if the stop request was issued, `false` if the stop request was already issued.
    auto request_stop() noexcept -> bool;

    /// \brief The `stop_requested()` method checks if a stop request has been issued.
    /// \return `true` if a stop request has been issued, `false` otherwise.
    [[nodiscard]] auto stop_requested() const noexcept -> bool;

   private:
    template <class>
    friend class inplace_stop_callback;
    friend class detail::inplace_stop_callback_base;
    friend class inplace_stop_token;

    auto lock() const noexcept -> uint8_t;
    auto unlock(uint8_t old_state) const noexcept -> void;

    auto try_lock(bool) const noexcept -> bool;
    auto try_add_callback(detail::inplace_stop_callback_base *) const noexcept -> bool;
    auto remove_callback(const detail::inplace_stop_callback_base *) const noexcept -> void;

   private:
    mutable detail::inplace_stop_callback_base *callbacks{nullptr};
    std::thread::id notifying{};
    mutable std::atomic<uint8_t> state{0};
  };

  /// \ingroup stop_token
  /// \brief The `mtc::inplace_stop_token` class provides the means to check if a stop request has been issued or can
  /// be issued, for its associated `mtc::inplace_stop_source`.
  class inplace_stop_token {
   public:
    template <class Fn>
    using callback_type = inplace_stop_callback<Fn>;

    inplace_stop_token() noexcept = default;
    inplace_stop_token(const inplace_stop_token &) noexcept = default;
    inplace_stop_token(inplace_stop_token &&other) noexcept : source{other.source} { other.source = nullptr; }
    ~inplace_stop_token() noexcept = default;

   public:
    auto operator=(const inplace_stop_token &) noexcept -> inplace_stop_token & = default;
    auto operator=(inplace_stop_token &&other) noexcept -> inplace_stop_token & {
      if (this != &other) {
        source = other.source;
        other.source = nullptr;
      }

      return *this;
    }
    auto operator==(const inplace_stop_token &other) const noexcept -> bool = default;

   public:
    /// \brief The `stop_requested()` method checks if a stop request has been issued.
    /// \return `true` if a stop request has been issued, `false` otherwise.
    [[nodiscard]] auto stop_requested() const noexcept -> bool { return stop_possible() && source->stop_requested(); }

    /// \brief The `stop_possible()` method checks if a stop request can be issued.
    /// \return `true` if a stop request can be issued, `false` otherwise.
    [[nodiscard]] auto stop_possible() const noexcept -> bool { return source != nullptr; }

   public:
    /// \brief The `swap()` method swaps the contents of two `mtc::inplace_stop_token` objects.
    /// \param other The other `mtc::inplace_stop_token` object to swap with.
    auto swap(inplace_stop_token &other) noexcept -> void { std::swap(source, other.source); }

   private:
    template <class>
    friend class inplace_stop_callback;
    friend class inplace_stop_source;

    explicit inplace_stop_token(const inplace_stop_source *src) noexcept : source{src} {}

   private:
    const inplace_stop_source *source{nullptr};
  };

  /// \ingroup stop_token
  /// \brief The `mtc::inplace_stop_callback` class provides the means to register a callback that is invoked when a stop
  /// request is issued to the associated `mtc::inplace_stop_source`.
  /// \tparam Fn The type of the callback function.
  template <class Fn>
  class inplace_stop_callback : detail::inplace_stop_callback_base {
   public:
    template <class F>
      requires constructible_from<Fn, F>
    explicit inplace_stop_callback(const inplace_stop_token token, F &&cb) noexcept(nothrow_constructible_from<Fn, F>)
        : inplace_stop_callback_base(token.source, &inplace_stop_callback::invoke_impl), callback{MTC_FWD(cb)} {
      register_callback();
    }

    ~inplace_stop_callback() noexcept {
      if (source != nullptr) source->remove_callback(this);
    }

   private:
    static auto invoke_impl(inplace_stop_callback_base *callback_base) noexcept -> void {
      MTC_MOVE(static_cast<inplace_stop_callback *>(callback_base)->callback)();
    }

    Fn callback;
  };

  template <class Fn>
  inplace_stop_callback(inplace_stop_token, Fn &&) -> inplace_stop_callback<Fn>;

}  // namespace mtc

#endif  // MTC_STOP_TOKEN_HPP
