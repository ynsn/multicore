// Copyright (c) 2024 - present, Yoram Janssen
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

#ifndef MTC_TUPLE_HPP
#define MTC_TUPLE_HPP

#include "config.hpp"
#include "meta.hpp"

#include <type_traits>
#include <utility>

namespace mtc {

  /**
   * \defgroup tuple tuple
   * \ingroup utility
   * \brief The `tuple` module provides compile-time programming utilities for working with tuples.
   */

  template <class...>
  struct tuple;

  namespace detail {
    template <size_t Inx, class Head, class... Tail>
    struct tuple_element_impl {
      using type = typename tuple_element_impl<Inx - 1, Tail...>::type;
    };

    template <class Head, class... Tail>
    struct tuple_element_impl<0, Head, Tail...> {
      using type = Head;
    };
  } // namespace detail

  template <size_t Inx, class Tuple>
  struct tuple_element;

  template <size_t Inx, class... Ts>
  struct tuple_element<Inx, mtc::tuple<Ts...>> {
    using type = typename detail::tuple_element_impl<Inx, Ts...>::type;
  };

  template <size_t Inx, class Tuple>
  using tuple_element_t = typename tuple_element<Inx, Tuple>::type;

  namespace detail {
    template <class T>
    struct tuple_storage_impl {
      T value;
    };

    template <class T>
      requires std::is_empty_v<T>
    struct tuple_storage_impl<T> : T {};

    template <size_t Idx, class T>
    struct tuple_storage : mtc::detail::tuple_storage_impl<T> {
      constexpr tuple_storage() noexcept = default;
      constexpr tuple_storage(T&& t) noexcept : mtc::detail::tuple_storage_impl<T>{MTC_FWD(t)} {}

      constexpr auto get() noexcept -> T& { return this->value; }
      constexpr auto get() const noexcept -> const T& { return this->value; }
    };

    template <class IndexSequence, class... Ts>
    struct tuple_impl;

    template <size_t... Is, class... Ts>
    struct tuple_impl<mtc::index_sequence<Is...>, Ts...> : mtc::detail::tuple_storage<Is, Ts>... {
      constexpr tuple_impl() noexcept = default;
      constexpr tuple_impl(Ts&&... ts) noexcept : mtc::detail::tuple_storage<Is, Ts>{MTC_FWD_T(Ts, ts)}... {}

      template <size_t Idx>
      constexpr auto& get() noexcept {
        return mtc::detail::tuple_storage<Idx, tuple_element_t<Idx, mtc::tuple<Ts...>>>::get();
      }

      template <size_t Idx>
      constexpr const auto& get() const noexcept {
        return mtc::detail::tuple_storage<Idx, tuple_element_t<Idx, mtc::tuple<Ts...>>>::get();
      }
    };
  } // namespace detail

  template <class... Ts>
  struct tuple : mtc::detail::tuple_impl<mtc::make_index_sequence<sizeof...(Ts)>, Ts...> {
    using base = mtc::detail::tuple_impl<mtc::make_index_sequence<sizeof...(Ts)>, Ts...>;
    constexpr tuple() noexcept = default;

    template <class... Args>
      requires(std::is_constructible_v<Ts, Args> && ...)
    constexpr tuple(Args&&... ts) noexcept : base{MTC_FWD_T(Ts, ts)...} {}
  };

  template <class... Ts>
  [[nodiscard]] constexpr auto make_tuple(Ts&&... ts) -> mtc::tuple<std::remove_reference_t<Ts>...> {
    return mtc::tuple<std::remove_reference_t<Ts>...>{MTC_FWD(ts)...};
  }

} // namespace mtc

#endif // MTC_TUPLE_HPP
