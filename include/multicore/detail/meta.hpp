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

#ifndef MTC_META_HPP
#define MTC_META_HPP

namespace mtc {

  /**
   * \defgroup meta meta
   * \ingroup utility
   * \brief The `meta` module provides compile-time programming utilities.
   */

  /// \ingroup meta
  /// \brief Concept that is always satisfied.
  ///
  /// The `mtc::ok` concept is a variadic template concept that always evaluates to true.
  /// It can be used as a default concept in template metaprogramming to ensure that
  /// a set of types meets a condition without imposing any specific constraints.
  ///
  /// \tparam Args The types.
  template <class... Args>
  concept ok = true;

  /// \ingroup meta
  /// \brief Alias template to extract the nested `type` from a given type `T`.
  ///
  /// The `mtc::type` alias template is a convenient utility that extracts the nested `type` member from a given type `T`.
  /// This is particularly useful in template metaprogramming where you need to work with nested types in a generic manner.
  ///
  /// \tparam T The type from which to extract the nested `type`.
  /// \pre `T` must have a nested `type` member.
  ///
  /// \details
  /// The `mtc::type` alias template simplifies the process of accessing nested types within template classes or structs.
  /// By using this alias, you can avoid repetitive and verbose syntax, making your code more readable and maintainable.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// struct Example {
  ///     using type = int;
  /// };
  ///
  /// mtc::type<Example> value = 42; // value is of type int
  /// \endcode
  ///
  /// In the above example, `mtc::type<Example>` resolves to `int`, allowing you to use `value` as an `int`.
  template <class T>
    requires requires { typename T::type; }
  using type = typename T::type;
  template <class T>
  inline constexpr auto value = T::value;

  /// \ingroup meta
  /// \brief A compile-time sequence of indices.
  ///
  /// The `mtc::index_sequence` struct is a utility that represents a compile-time sequence of indices.
  /// It is commonly used in template metaprogramming to generate sequences of indices for parameter packs.
  ///
  /// \tparam Is The indices in the sequence.
  ///
  /// \details
  /// The `mtc::index_sequence` struct is particularly useful when working with variadic templates.
  /// It allows you to generate a sequence of indices that can be used to access elements in a parameter pack.
  /// This can be helpful in scenarios where you need to perform operations on each element of a parameter pack
  /// in a compile-time loop.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// template <size_t... Is>
  /// void print_indices(index_sequence<Is...>) {
  ///     ((std::cout << Is << ' '), ...);
  /// }
  ///
  /// int main() {
  ///     print_indices(index_sequence<0, 1, 2, 3>{});
  ///     return 0;
  /// }
  /// \endcode
  ///
  /// In the above example, `print_indices` prints the indices `0 1 2 3` to the standard output.
  template <size_t ...Is>
  struct index_sequence {};

  namespace detail {
    template <size_t N, size_t... Is>
    struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};

    template <size_t... Is>
    struct make_index_sequence_impl<0, Is...> {
      using type = index_sequence<Is...>;
    };
  } // namespace detail

  /// \ingroup meta
  /// \brief Alias template to generate a compile-time sequence of indices.
  ///
  /// The `mtc::make_index_sequence` alias template generates a compile-time sequence of indices from `0` to `N-1`.
  /// It is a utility commonly used in template metaprogramming to create index sequences for parameter packs.
  ///
  /// \tparam N The number of indices in the sequence.
  ///
  /// \details
  /// The `mtc::make_index_sequence` alias template simplifies the process of generating index sequences for variadic templates.
  /// By using this alias, you can create a sequence of indices that can be used to access elements in a parameter pack,
  /// enabling compile-time iteration over the pack.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// template <size_t... Is>
  /// void print_indices(mtc::index_sequence<Is...>) {
  ///     ((std::cout << Is << ' '), ...);
  /// }
  ///
  /// int main() {
  ///     print_indices(mtc::make_index_sequence<4>{});
  ///     return 0;
  /// }
  /// \endcode
  ///
  /// In the above example, `print_indices` prints the indices `0 1 2 3` to the standard output.
  template <size_t N>
  using make_index_sequence = mtc::type<detail::make_index_sequence_impl<N>>;

  template <class Fn, class... Args>
  using calln = typename Fn::template call<Args...>;

  template <class Fn, class Arg1>
  using call1 = typename Fn::template call<Arg1>;

  template <class Fn, class Arg1, class Arg2>
  using call2 = typename Fn::template call<Arg1, Arg2>;

  template <template <class...> class Fn>
  struct quoten {
    template <class... Args>
    using call = Fn<Args...>;
  };

  template <template <class...> class Fn>
  struct quote1 {
    template <class Arg1>
    using call = Fn<Arg1>;
  };

  template <template <class...> class Fn>
  struct quote2 {
    template <class Arg1, class Arg2>
    using call = Fn<Arg1, Arg2>;
  };

  template <bool Cond>
  struct select {
    template <class True, class...>
    using call = True;
  };

  template <>
  struct select<false> {
    template <class, class False>
    using call = False;
  };

  template <bool Cond, class Then = void, class... Else>
  using select_t = mtc::calln<mtc::select<Cond>, Then, Else...>;

} // namespace mtc

#endif // MTC_META_HPP
