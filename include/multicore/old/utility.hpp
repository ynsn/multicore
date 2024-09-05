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

#ifndef MTC_UTILITY_HPP
#define MTC_UTILITY_HPP

#include "concepts.hpp"

#include <type_traits>
#include <string_view>

#define MTC_FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)
#define MTC_FWD_T(t, ...) static_cast<t &&>(__VA_ARGS__)
#define MTC_MOVE(...) static_cast<typename mtc::detail::remove_reference<decltype(__VA_ARGS__)>::type &&>(__VA_ARGS__)

namespace mtc {

  /**
   * \defgroup utility utility
   * \brief The `utility` module provides utilities used throughout the library.
   */

  namespace detail {
    template <class T>
    struct remove_reference {
      using type = T;
    };

    template <class T>
    struct remove_reference<T &> {
      using type = T;
    };

    template <class T>
    struct remove_reference<T &&> {
      using type = T;
    };

    template <class>
    inline constexpr auto always_false = false;


  }  // namespace detail

  template <template <class> class>
  struct template_alias {};

  template <class T>
  auto as_lvalue(T &&) -> T &;

  /// \ingroup utility
  /// \brief The `mtc::declval` function converts any type `T` to an rvalue reference of type `T`, making it possible to use
  /// member functions in the operand of the `decltype` specifier without the need to construct an instance of `T`. Can
  /// only be used in unevaluated contexts.
  /// \tparam T The type to convert to an rvalue reference.
  /// \return Cannot be called and thus never returns a value. The return type is `T &&`.
  template <class T>
  constexpr auto declval() noexcept -> T && {
    static_assert(detail::always_false<T>, "Calling declval is ill-formed");
  }

  /// \ingroup utility
  /// \brief The `mtc::exchange` function replaces the value of `obj` with `new_value` and returns the old value of `obj`.
  /// \tparam T The type of the object to replace.
  /// \tparam U The type of the new value.
  /// \param obj A reference to the object whose value to replace.
  /// \param new_value The new value to assign to `obj`.
  /// \return The old value of `obj`.
  template <class T, class U = T>
    requires std::is_move_constructible_v<T> && std::is_assignable_v<T &, U>
  constexpr auto exchange(T &obj, U &&new_value) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_assignable_v<T &, U>) -> T {
    T old_value = MTC_MOVE(obj);
    obj = MTC_FWD(new_value);
    return old_value;
  }

  template <class Fn, class... Args>
  using call_result_t = decltype(declval<Fn>()(declval<Args>()...));

  struct none_t {};
  inline constexpr auto none = none_t{};

  template <template <class...> class Tag>
  static constexpr auto get_value_with_in() noexcept -> none_t {
    return none;
  }

  template <template <class...> class Tag, class Arg, class... Args>
  [[nodiscard]] static constexpr auto get_value_with_in(Arg &&arg, Args &&...args) noexcept -> decltype(auto) {
    if constexpr (instance_of<std::decay_t<Arg>, Tag>) {
      return MTC_FWD(arg);
    } else {
      return get_value_with_in<Tag>(MTC_FWD(args)...);
    }
  }

  template <class Fn>
  struct scope_guard : Fn {
    constexpr explicit scope_guard(Fn &&fn) noexcept : Fn(MTC_MOVE(fn)) {}
    constexpr scope_guard(const scope_guard &) = delete;
    constexpr scope_guard(scope_guard &&) = delete;
    constexpr ~scope_guard() { this->operator()(); }

    constexpr auto operator=(const scope_guard &) -> scope_guard & = delete;
    constexpr auto operator=(scope_guard &&) -> scope_guard & = delete;
  };



}  // namespace mtc

#endif  // MTC_UTILITY_HPP
