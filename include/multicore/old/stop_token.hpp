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

#include <concepts>
#include <thread>

namespace mtc {

  /**
   * \defgroup stop_token stop_token
   * \brief The `stop_token` module provides facilities for working with stop tokens.
   */

  /// \ingroup stop_token
  /// \brief The `mtc::stoppable_token` concept is satisfied if `Token` models a stop token that is copyable, equality
  /// comparable, swappable and allows polling to see if a stop request has been issued and/or is at all possible.
  /// \tparam Token The type to check.
  template <class Token>
  concept stoppable_token = requires(const Token tok) {
    { tok.stop_requested() } noexcept -> std::same_as<bool>;
    { tok.stop_possible() } noexcept -> std::same_as<bool>;
    { Token(tok) } noexcept;
    typename detail::template_alias<Token::template callback_type>;
  } && std::copyable<Token> && std::equality_comparable<Token> && std::swappable<Token>;

  /// \ingroup stop_token
  /// \brief The `mtc::stoppable_token_for` concept is satisfied if `Token` satisfies the `mtc::stoppable_token` concept
  /// and `Callback` is compatible with the token's callback type.
  /// \tparam Token The type to check.
  /// \tparam Callback The type to check.
  /// \tparam Init Initializer.
  template <class Token, class Callback, class Init = Callback>
  concept stoppable_token_for = stoppable_token<Token> &&                                                                            //
                                std::invocable<Callback> &&                                                                          //
                                requires { typename Token::template callback_type<Callback>; } &&                                    //
                                std::constructible_from<Callback, Init> &&                                                           //
                                std::constructible_from<typename Token::template callback_type<Callback>, Token, Init> &&            //
                                std::constructible_from<typename Token::template callback_type<Callback>, Token &, Callback> &&      //
                                std::constructible_from<typename Token::template callback_type<Callback>, const Token, Callback> &&  //
                                std::constructible_from<typename Token::template callback_type<Callback>, const Token &, Callback>;  //

  /// \ingroup stop_token
  /// \brief The `mtc::unstoppable_token` concept is satisfied if `Token` satisfies the `mtc::stoppable_token` concept
  /// but disallows issuing stop requests.
  /// \tparam Token The type to check.
  template <class Token>
  concept unstoppable_token = stoppable_token<Token> &&  //
                              requires {
                                { Token::stop_possible() } -> boolean_testable;
                                requires(!Token::stop_possible());
                              };

  /// \ingroup stop_token
  /// \brief The `mtc::never_stop_token` class provides a stop token that never allows issuing stop requests.
  class never_stop_token {
   private:
    struct callback {
      constexpr explicit callback(never_stop_token, auto &&) noexcept {}
    };

   public:
    template <class>
    using callback_type = callback;

    [[nodiscard]] static constexpr auto stop_possible() noexcept -> bool { return false; }
    [[nodiscard]] static constexpr auto stop_requested() noexcept -> bool { return false; }
    [[nodiscard]] friend constexpr auto operator==(const never_stop_token &, const never_stop_token &) noexcept -> bool { return true; }
    [[nodiscard]] friend constexpr auto operator!=(never_stop_token, never_stop_token) noexcept -> bool = default;
  };

  class inplace_stop_token;
  class inplace_stop_source;
  template <class Fn>
  class inplace_stop_callback;

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

  /// \ingroup stop_token
  /// \brief The `mtc::inplace_stop_source` class provides the means to issue stop requests to associated stop tokens.
  /// Contrary to `std::stop_source`, this class guarantees that it does not dynamically allocate memory on the heap.
  class inplace_stop_source {
   public:
    inplace_stop_source() noexcept = default;
    inplace_stop_source(const inplace_stop_source &) = delete;
    inplace_stop_source(inplace_stop_source &&) = delete;
    ~inplace_stop_source() noexcept;

    auto operator=(const inplace_stop_source &) -> inplace_stop_source & = delete;
    auto operator=(inplace_stop_source &&) -> inplace_stop_source & = delete;

   public:
    /// \brief The `request_stop()` method issues a stop request to all associated stop tokens if the
    /// `mtc::inplace_stop_source` object has not yet already received a stop request.
    /// \return `true` if the stop request was issued, otherwise `false`.
    /// \post `stop_requested() == true`
    auto request_stop() noexcept -> bool;

    auto swap(inplace_stop_source &other) noexcept -> void = delete;

   public:
    /// \brief The `get_token()` method returns a new stop token associated with this stop source.
    /// \return A new stop token.
    [[nodiscard]] auto get_token() const noexcept -> inplace_stop_token;

    /// \brief The `stop_requested()` method checks if a stop request has been issued.
    /// \return `true` if a stop request has been issued, `false` otherwise.
    [[nodiscard]] auto stop_requested() const noexcept -> bool;

    /// \brief The `stop_possible()` method returns whether this object may issue a stop request.
    /// \return `true` if a stop request may be issued, `false` otherwise.
    [[nodiscard]] static constexpr auto stop_possible() noexcept -> bool { return true; }

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
    inplace_stop_token(inplace_stop_token &&other) noexcept;
    ~inplace_stop_token() noexcept = default;

   public:
    auto operator=(const inplace_stop_token &) noexcept -> inplace_stop_token & = default;
    auto operator=(inplace_stop_token &&other) noexcept -> inplace_stop_token &;
    auto operator==(const inplace_stop_token &other) const noexcept -> bool = default;

   public:
    /// \brief The `stop_requested()` method checks if a stop request has been issued.
    /// \return `true` if a stop request has been issued, `false` otherwise.
    [[nodiscard]] auto stop_requested() const noexcept -> bool;

    /// \brief The `stop_possible()` method checks if a stop request can be issued.
    /// \return `true` if a stop request can be issued, `false` otherwise.
    [[nodiscard]] auto stop_possible() const noexcept -> bool;

   public:
    /// \brief The `swap()` method swaps the contents of two `mtc::inplace_stop_token` objects.
    /// \param other The other `mtc::inplace_stop_token` object to swap with.
    auto swap(inplace_stop_token &other) noexcept -> void;

   private:
    template <class>
    friend class inplace_stop_callback;
    friend class inplace_stop_source;

    explicit inplace_stop_token(const inplace_stop_source *src) noexcept;

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
      requires std::constructible_from<Fn, F>
    explicit inplace_stop_callback(const inplace_stop_token token, F &&cb) noexcept(std::is_nothrow_constructible_v<Fn, F>)
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
