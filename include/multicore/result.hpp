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

#ifndef MTC_RESULT_HPP
#define MTC_RESULT_HPP

#include "concepts.hpp"
#include "utility.hpp"

#include <cassert>
#include <initializer_list>
#include <new>
#include <type_traits>

namespace mtc {

  /**
   * \defgroup result result
   * \brief The `result` module provides error handling facilities replacing C++ exceptions.
   */

  struct monostate {
    constexpr monostate() noexcept = default;
  };

  struct in_place_value_t {
    constexpr in_place_value_t() noexcept = default;
  };

  struct in_place_error_t {
    constexpr in_place_error_t() noexcept = default;
  };

  inline constexpr auto in_place_value = in_place_value_t{};
  inline constexpr auto in_place_error = in_place_error_t{};

  template <class Value>
  concept value_type = true;  // /*std::is_void_v<Value> ||*/ (std::is_nothrow_constructible_v<std::remove_cvref_t<Value>, Value&>);

  /// \ingroup result
  /// \brief The `error_type` concept is satisfied by types that are nothrow default constructible, not `void` and not a reference.
  template <class Error>
  concept error_type =                                   //
      !std::is_void_v<Error> &&                          // Error type must not be `void`.
      std::is_nothrow_default_constructible_v<Error> &&  // Error type must be nothrow default constructible.
      (std::is_nothrow_copy_assignable_v<Error> || std::is_nothrow_move_constructible_v<Error>);

  template <value_type V, error_type E>
  class result;

  /// \ingroup result
  /// \brief The `failure<E>` wrapper type wraps a satisfies `error_type` E.
  template <error_type E>
  class failure {
   public:
    template <value_type, error_type>
    friend class result;

    using error_type = E;                                              ///< The wrapped error type.
    using lvalue_reference = std::add_lvalue_reference_t<error_type>;  ///< The wrapped error type a a lvalue reference.
    using rvalue_reference = std::add_rvalue_reference_t<error_type>;
    using const_lvalue_reference = std::add_lvalue_reference_t<std::add_const_t<error_type>>;
    using const_rvalue_reference = std::add_rvalue_reference_t<std::add_const_t<error_type>>;

    //-------------------------------------------------------------------------------------------------------------------------------------------
    // Constructors
    //-------------------------------------------------------------------------------------------------------------------------------------------
    constexpr failure() noexcept
      requires(std::is_default_constructible_v<error_type>)
    = default;
    constexpr failure()
      requires(!std::is_default_constructible_v<error_type>)
    = delete;

    template <class Err>
      requires constructible_from<E, Err &&> && (!same_as<std::remove_cvref_t<Err>, in_place_error_t>) &&
               (!same_as<std::remove_cvref_t<Err>, failure>)
    constexpr explicit(!std::convertible_to<Err &&, error_type>) failure(Err &&err) noexcept(noexcept(error_type(static_cast<Err &&>(err))))
        : error_value(static_cast<Err &&>(err)) {}

    template <class... Args>
      requires constructible_from<error_type, Args...>
    constexpr explicit failure(in_place_error_t, Args &&...args) noexcept(noexcept(error_type(static_cast<Args &&>(args)...)))
        : error_value(static_cast<Args &&>(args)...) {}

    template <class U, class... Args>
      requires constructible_from<error_type, std::initializer_list<U> &, Args...>
    constexpr explicit failure(in_place_error_t, std::initializer_list<U> ilist,
                               Args &&...args) noexcept(noexcept(error_type(ilist, static_cast<Args &&>(args)...)))
        : error_value(ilist, static_cast<Args &&>(args)...) {}

    constexpr failure(const failure &) noexcept(std::is_nothrow_copy_constructible_v<error_type>)
      requires(std::is_copy_constructible_v<error_type>)
    = default;
    constexpr failure(const failure &)
      requires(!std::is_copy_constructible_v<error_type>)
    = delete;

    template <class U>
      requires constructible_from<error_type, const U &> && (!same_as<U, error_type>)
    constexpr explicit(!std::convertible_to<const U &, error_type>)
        failure(const failure<U> &other) noexcept(noexcept(error_type(other.value())))
        : error_value(other.value()) {}

    constexpr failure(failure &&) noexcept(std::is_nothrow_move_constructible_v<error_type>)
      requires(std::is_move_constructible_v<error_type>)
    = default;
    constexpr failure(failure &&) noexcept(std::is_nothrow_move_constructible_v<error_type>)
      requires(!std::is_move_constructible_v<error_type>)
    = delete;

    template <class U>
      requires constructible_from<error_type, U &&> && (!same_as<U, error_type>)
    constexpr explicit(!std::convertible_to<U &&, error_type>) failure(failure<U> &&other) noexcept(noexcept(error_type(other.value())))
        : error_value(std::move(other.value())) {}

    constexpr auto operator=(const failure &other) noexcept(std::is_nothrow_copy_assignable_v<error_type>) -> failure &
      requires(std::is_copy_assignable_v<error_type>)
    = default;
    constexpr auto operator=(const failure &other) noexcept(std::is_nothrow_copy_assignable_v<error_type>) -> failure &
      requires(!std::is_copy_assignable_v<error_type>)
    = delete;

    template <class U>
      requires std::convertible_to<const U &, error_type> &&
               !same_as<U, error_type>
               constexpr auto operator=(const failure<U> &other) noexcept(noexcept(error_value = other.value()))->failure & {
      error_value = other.value();
      return *this;
    }

    constexpr auto operator=(failure &&other) noexcept(std::is_nothrow_move_assignable_v<error_type>) -> failure &
      requires(std::is_move_assignable_v<error_type>)
    = default;
    constexpr auto operator=(failure &&other) noexcept(std::is_nothrow_move_assignable_v<error_type>) -> failure &
      requires(!std::is_move_assignable_v<error_type>)
    = delete;

    template <class U>
      requires std::convertible_to<U &&, error_type> &&
               !same_as<U, error_type>
               constexpr auto operator=(failure<U> &&other) noexcept(noexcept(error_value = std::move(other.value())))->failure & {
      error_value = std::move(other.value());
      return *this;
    }

    template <class Err>
      requires std::convertible_to<Err, E> && (!same_as<std::remove_cvref_t<Err>, in_place_error_t>) &&
               (!same_as<std::remove_cvref_t<Err>, failure<E>>)
    constexpr auto operator=(Err &&other) noexcept(noexcept(error_value = static_cast<Err &&>(other))) -> failure & {
      error_value = static_cast<Err &&>(other);
      return *this;
    }

    constexpr ~failure() noexcept(std::is_nothrow_destructible_v<error_type>)
      requires(std::is_destructible_v<error_type>)
    = default;

    [[nodiscard]] constexpr auto value() & noexcept -> lvalue_reference { return error_value; }
    [[nodiscard]] constexpr auto value() const & noexcept -> const_lvalue_reference { return error_value; }
    [[nodiscard]] constexpr auto value() && noexcept -> rvalue_reference { return static_cast<rvalue_reference>(error_value); }
    [[nodiscard]] constexpr auto value() const && noexcept -> const_rvalue_reference { return static_cast<const_rvalue_reference>(error_value); }

   private:
    error_type error_value;
  };

  template <class E>
  failure(E) -> failure<E>;

  template <value_type V, error_type E>
  class result {
   public:
    template <value_type, error_type>
    friend class result;
    using value_type = V;
    using error_type = E;

   private:
    using wrapped_type =
        std::conditional_t<std::is_void_v<V>, monostate, std::conditional_t<std::is_reference_v<V>, std::remove_reference_t<V> *, V>>;

    using value_lvalue_reference = std::add_lvalue_reference_t<value_type>;
    using value_rvalue_reference = std::add_rvalue_reference_t<value_type>;
    using value_const_lvalue_reference = std::add_lvalue_reference_t<std::add_const_t<value_type>>;
    using value_const_rvalue_reference = std::add_rvalue_reference_t<std::add_const_t<value_type>>;

    using error_lvalue_reference = std::add_lvalue_reference_t<error_type>;
    using error_rvalue_reference = std::add_rvalue_reference_t<error_type>;
    using error_const_lvalue_reference = std::add_lvalue_reference_t<std::add_const_t<error_type>>;
    using error_const_rvalue_reference = std::add_rvalue_reference_t<std::add_const_t<error_type>>;

   public:
    constexpr result() noexcept(std::is_nothrow_default_constructible_v<value_type>)
      requires std::is_default_constructible_v<value_type> || std::is_void_v<value_type>
        : active(true) {
      new (static_cast<void *>(&val)) wrapped_type();
    }

    constexpr result() noexcept(std::is_nothrow_default_constructible_v<value_type>)
      requires(!std::is_default_constructible_v<value_type>) && (!std::is_void_v<value_type>)
    = delete;

    //-------------------------------------------------------------------------------------------------------------------------------------------
    // Copy constructors
    //-------------------------------------------------------------------------------------------------------------------------------------------
   public:
    constexpr result(const result &) noexcept(std::is_nothrow_copy_constructible_v<wrapped_type> &&
                                              std::is_nothrow_copy_constructible_v<error_type>)
      requires std::is_trivially_copy_constructible_v<wrapped_type> && std::is_trivially_copy_constructible_v<error_type>
    = default;

    constexpr result(const result &) noexcept(std::is_nothrow_copy_constructible_v<wrapped_type> &&
                                              std::is_nothrow_copy_constructible_v<error_type>)
      requires(!std::is_copy_constructible_v<wrapped_type>) || (!std::is_copy_constructible_v<error_type>)
    = delete;

    constexpr result(const result &other) noexcept(std::is_nothrow_copy_constructible_v<wrapped_type> &&
                                                   std::is_nothrow_copy_constructible_v<error_type>)
        : active(other.active) {
      if (other.has_value()) {
        new (static_cast<void *>(&val)) wrapped_type(other.val);
      } else {
        new (static_cast<void *>(&err)) error_type(other.err);
      }
    }

    template <class T, class U>
      requires constructible_from<value_type, const T &> &&  //
               constructible_from<error_type, const U &>     //
    constexpr explicit(!std::convertible_to<const T &, value_type> && !std::convertible_to<const U &, error_type>)
        result(const result<T, U> &other) noexcept(std::is_nothrow_constructible_v<value_type, const T &> &&
                                                   std::is_nothrow_constructible_v<error_type, const U &>)
        : active(other.active) {
      if (other.has_value()) {
        new (static_cast<void *>(&val)) wrapped_type(other.val);
      } else {
        new (static_cast<void *>(&err)) error_type(other.err);
      }
    }

    // Move constructors

    constexpr result(result &&) noexcept(std::is_nothrow_move_constructible_v<wrapped_type> && std::is_nothrow_move_constructible_v<error_type>)
      requires std::is_trivially_move_constructible_v<wrapped_type> && std::is_trivially_move_constructible_v<error_type>
    = default;

    constexpr result(result &&) noexcept(std::is_nothrow_move_constructible_v<wrapped_type> && std::is_nothrow_move_constructible_v<error_type>)
      requires(!std::is_move_constructible_v<wrapped_type>) || (!std::is_move_constructible_v<error_type>)
    = delete;

    constexpr result(result &&other) noexcept(std::is_nothrow_move_constructible_v<wrapped_type> &&
                                              std::is_nothrow_move_constructible_v<error_type>)
        : active(other.active) {
      if (other.has_value()) {
        new (static_cast<void *>(&val)) wrapped_type(std::move(other.val));
      } else {
        new (static_cast<void *>(&err)) error_type(std::move(other.err));
      }
    }

    template <class T, class U>
      requires constructible_from<value_type, T &&> &&  //
               constructible_from<error_type, U &&>     //
    constexpr explicit(!std::convertible_to<T &&, value_type> && !std::convertible_to<U &&, error_type>)
        result(result<T, U> &&other) noexcept(std::is_nothrow_constructible_v<value_type, T &&> &&
                                              std::is_nothrow_constructible_v<error_type, U &&>)
        : active(other.active) {
      if (other.has_value()) {
        new (static_cast<void *>(&val)) wrapped_type(std::move(other.val));
      } else {
        new (static_cast<void *>(&err)) error_type(std::move(other.err));
      }
    }

    // Copy assignment

    constexpr auto operator=(const result &) noexcept(std::is_nothrow_copy_assignable_v<value_type> &&
                                                      std::is_nothrow_copy_assignable_v<error_type>) -> result &
      requires std::is_trivially_copy_assignable_v<value_type> && std::is_trivially_copy_assignable_v<error_type>
    = default;

    constexpr auto operator=(const result &) noexcept(std::is_nothrow_copy_assignable_v<value_type> &&
                                                      std::is_nothrow_copy_assignable_v<error_type>) -> result &
      requires(!std::is_copy_assignable_v<value_type>) || (!std::is_copy_assignable_v<error_type>)
    = delete;

    constexpr auto operator=(const result &other) noexcept(std::is_nothrow_copy_assignable_v<value_type> &&
                                                           std::is_nothrow_copy_assignable_v<error_type>) -> result & = delete;

    constexpr auto operator=(result &&other) noexcept(std::is_nothrow_copy_assignable_v<value_type> &&
                                                      std::is_nothrow_copy_assignable_v<error_type>) -> result & = delete;

    template <class T = V>
      requires(!std::is_void_v<T>) &&                                      //
                  constructible_from<value_type, T &&> &&                  //
                  (!same_as<std::remove_cvref_t<T>, in_place_value_t>) &&  //
                  (!same_as<std::remove_cvref_t<T>, in_place_error_t>) &&  //
                  (!same_as<std::remove_cvref_t<T>, result>) &&            //
                  (!same_as<std::remove_cvref_t<T>, failure<error_type>>)  //
    constexpr explicit(!std::convertible_to<T &&, value_type>) result(T &&t) noexcept(noexcept(value_type(static_cast<T &&>(t))))
        : active(true), empty{} {
      if constexpr (std::is_reference_v<value_type>) {
        new (static_cast<void *>(&val)) wrapped_type(&t);
      } else {
        new (static_cast<void *>(&val)) wrapped_type(static_cast<T &&>(t));
      }
    }

    template <class T = error_type>
      requires constructible_from<error_type, const T &>  //
    constexpr explicit(!std::convertible_to<const T &, error_type>) result(const failure<T> &t) noexcept(noexcept(error_type(t.value())))
        : active(false), empty{} {
      new (static_cast<void *>(&err)) error_type(t.value());
    }

    template <class... Args>
      requires constructible_from<value_type, Args...>
    constexpr explicit result(in_place_value_t, Args &&...args) noexcept(noexcept(value_type(static_cast<Args &&>(args)...))) : active(true) {
      if constexpr (std::is_reference_v<value_type>) {
        new (static_cast<void *>(&val)) wrapped_type(&static_cast<Args &&>(args)...);
      } else {
        new (static_cast<void *>(&val)) wrapped_type(static_cast<Args &&>(args)...);
      }
    }

    template <class U, class... Args>
      requires constructible_from<value_type, std::initializer_list<U> &, Args...>
    constexpr explicit result(in_place_value_t, std::initializer_list<U> ilist,
                              Args &&...args) noexcept(noexcept(value_type(ilist, static_cast<Args &&>(args)...)))
        : active(true) {
      new (static_cast<void *>(&val)) wrapped_type(ilist, static_cast<Args &&>(args)...);
    }

    constexpr ~result() noexcept(std::is_nothrow_destructible_v<wrapped_type> && std::is_nothrow_destructible_v<error_type>)
      requires std::is_trivially_destructible_v<wrapped_type> && std::is_trivially_destructible_v<error_type>
    = default;
    constexpr ~result() noexcept(std::is_nothrow_destructible_v<wrapped_type> && std::is_nothrow_destructible_v<error_type>) {
      if (active) {
        if constexpr (!std::is_trivially_destructible_v<wrapped_type>) val.~wrapped_type();
      } else {
        if constexpr (!std::is_trivially_destructible_v<error_type>) err.~error_type();
      }
    }

    [[nodiscard]] constexpr operator bool() const noexcept { return has_value(); }
    [[nodiscard]] constexpr auto operator*() noexcept -> decltype(auto) { return value(); }

    /*[[nodiscard]] constexpr auto operator*() const & noexcept -> value_const_lvalue_reference {
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return *val;
      } else {
        return val;
      }
    }

    [[nodiscard]] constexpr auto operator*() && noexcept -> value_rvalue_reference {
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return static_cast<value_rvalue_reference>(*val);
      } else {
        return static_cast<value_rvalue_reference>(val);
      }
    }

    [[nodiscard]] constexpr auto operator*() const && noexcept -> value_const_rvalue_reference {
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return static_cast<value_rvalue_reference>(*val);
      } else {
        return static_cast<value_rvalue_reference>(val);
      }
    }*/

    [[nodiscard]] constexpr auto has_value() const noexcept -> bool { return active; }
    [[nodiscard]] constexpr auto has_error() const noexcept -> bool { return !active; }

    [[nodiscard]] constexpr auto value() & noexcept -> value_lvalue_reference {
      assert(has_value() && "result must contain a value");
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return *val;
      } else {
        return val;
      }
    }
    [[nodiscard]] constexpr auto value() const & noexcept -> value_const_lvalue_reference {
      assert(has_value() && "result must contain a value");
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return *val;
      } else {
        return val;
      }
    }
    [[nodiscard]] constexpr auto value() && noexcept -> value_rvalue_reference {
      assert(has_value() && "result must contain a value");
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return static_cast<value_rvalue_reference>(*val);
      } else {
        return static_cast<value_rvalue_reference>(val);
      }
    }
    [[nodiscard]] constexpr auto value() const && noexcept -> value_const_rvalue_reference {
      assert(has_value() && "result must contain a value");
      if constexpr (std::is_void_v<value_type>) {
        return;
      } else if constexpr (std::is_reference_v<value_type>) {
        return static_cast<value_const_rvalue_reference>(*val);
      } else {
        return static_cast<value_const_rvalue_reference>(val);
      }
    }

    template <class T>
      requires(!std::is_void_v<value_type>) && std::convertible_to<T, value_type>
    [[nodiscard]] constexpr auto value_or(T &&v) & noexcept(noexcept(value_type(static_cast<T &&>(v)))) -> value_lvalue_reference {
      if (has_value()) return value();
      return static_cast<T &&>(v);
    }

    template <class T>
      requires(!std::is_void_v<value_type>) && std::convertible_to<T, value_type>
    [[nodiscard]] constexpr auto value_or(T &&v) && noexcept(noexcept(value_type(static_cast<T &&>(v)))) -> value_rvalue_reference {
      if (has_value()) return static_cast<value_rvalue_reference>(value());
      return static_cast<value_rvalue_reference>(v);
    }

    template <class Fn>
      requires std::is_invocable_r_v<value_type, Fn, result &&>
    [[nodiscard]] constexpr auto or_else(Fn &&fn) && noexcept -> value_rvalue_reference {
      if (has_value()) {
        return static_cast<value_rvalue_reference>(value());
      } else {
        return MTC_FWD(fn)(std::move(*this));
      }
    }

    [[nodiscard]] constexpr auto error() & noexcept -> error_lvalue_reference { return err; }
    [[nodiscard]] constexpr auto error() const & noexcept -> error_const_lvalue_reference { return err; }
    [[nodiscard]] constexpr auto error() && noexcept -> error_rvalue_reference { return static_cast<error_rvalue_reference>(err); }
    [[nodiscard]] constexpr auto error() const && noexcept -> error_const_rvalue_reference {
      return static_cast<error_const_rvalue_reference>(err);
    }

   private:
    bool active;
    union {
      monostate empty;
      wrapped_type val;
      error_type err;
    };
  };

  template <class E>
  [[nodiscard]] constexpr auto fail(E &&error) noexcept -> failure<E> {
    return failure<E>(static_cast<E &&>(error));
  }

  template <class E, class... Args>
    requires constructible_from<E, Args...>
  [[nodiscard]] constexpr auto fail(Args &&...args) noexcept -> failure<E> {
    return failure<E>(in_place_value, static_cast<Args &&>(args)...);
  }

  template <class V, class E, class Fn>
    requires std::is_invocable_v<Fn, result<V, E> &>
  [[nodiscard]] constexpr auto try_or(result<V, E> &result, Fn &&fn) noexcept -> V & {
    if (result.has_value()) return result.value();
    return MANI_FWD(fn)(result);
  }
}  // namespace mtc

#endif  // MTC_RESULT_HPP
