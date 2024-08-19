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

#ifndef MTC_ALLOCATOR_HPP
#define MTC_ALLOCATOR_HPP

#include "concepts.hpp"
#include "utility.hpp"

#include <type_traits>

namespace mtc {
  /**
   * \defgroup allocator allocator
   * \brief The `allocator` module provides concepts, types and functions for working with allocators.
   */

  /// \ingroup allocator
  /// \brief The `mtc::allocator` concept is satisfied if `Allocator` is a type that satisfies all of the requirements of a C++ Allocator.
  /// \tparam Allocator The type to check.
  /// \see https://en.cppreference.com/w/cpp/named_req/Allocator
  template <class Allocator>
  concept allocator =
      requires { typename std::remove_cvref_t<Allocator>::value_type; } &&  //
      requires(Allocator &&a, const Allocator &b, Allocator c, size_t n, typename std::remove_cvref_t<Allocator>::value_type *p) {
        { Allocator(b) } noexcept;                                                                          // Copy constructible
        { Allocator(MTC_MOVE(a)) } noexcept;                                                                // Move constructible
        { Allocator(MTC_MOVE(c)) } noexcept;                                                                // Regular constructible
        { MTC_FWD(a) = b } noexcept;                                                                        // Copy assignable
        { MTC_FWD(a) = MTC_MOVE(c) } noexcept;                                                              // Move assignable
        { MTC_FWD(a) == MTC_FWD(a) } noexcept -> std::convertible_to<bool>;                                 // Comparable
        { MTC_FWD(a) != MTC_FWD(a) } noexcept -> std::convertible_to<bool>;                                 // Comparable
        { MTC_FWD(a).allocate(n) } -> std::same_as<typename std::remove_cvref_t<Allocator>::value_type *>;  // `allocate` method
        { MTC_FWD(a).deallocate(p, n) } -> std::same_as<void>;                                              // `deallocate` method
      };

  /// \ingroup allocator
  /// \brief The `mtc::allocator_for` concept is satisfied if `Allocator` satisfies the `mtc::allocator` concept and allows allocation and
  /// deallocation of objects of type `T`.
  /// \tparam Allocator The type to check.
  /// \tparam T The type to allocate and deallocate.
  /// \see mtc::allocator
  /// \see https://en.cppreference.com/w/cpp/named_req/Allocator
  template <class Allocator, class T>
  concept allocator_for = allocator<Allocator> && std::same_as<T, typename std::remove_cvref_t<Allocator>::value_type>;

  /// \ingroup allocator
  /// \brief The `mtc::allocate_result_t` alias template is used to get the result type of the `allocate` method of an allocator.
  /// \tparam Allocator The allocator type.
  /// \see mtc::allocator
  template <allocator Allocator>
  using allocate_result_t = decltype(declval<Allocator>().allocate(declval<size_t>()));

  template <allocator Allocator>
  struct with_allocator {
    [[no_unique_address]] Allocator allocator;

    template <class U = Allocator>
      requires std::constructible_from<Allocator, U &&>
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr with_allocator(U &&u) noexcept : allocator(MTC_FWD(u)) {}  // NOLINT(*-explicit-constructor)
  };

  template <class... Args>
  struct allocator_in : std::type_identity<void> {};
  template <class T, class... Args>
  struct allocator_in<with_allocator<T>, Args...> : std::type_identity<T> {};
  template <class Head, class... Tail>
  struct allocator_in<Head, Tail...> : allocator_in<Tail...> {};

  template <class... Args>
  using allocator_in_t = typename allocator_in<Args...>::type;

  template <class... Args>
  inline constexpr auto using_allocator_v = !std::same_as<allocator_in_t<Args...>, void>;

  template <class... Args>
  concept using_allocator = !std::same_as<allocator_in_t<Args...>, void>;

}  // namespace mtc

#endif  // MTC_ALLOCATOR_HPP
