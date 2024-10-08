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

#include "detail/config.hpp"
#include "detail/meta.hpp"

#include <concepts>

namespace mtc {

  /**
   * \defgroup allocator allocator
   * \brief The `allocator` module provides facilities for working with allocators.
   */

  /// \concept mtc::allocator
  /// \ingroup allocator
  /// \brief Specifies the requirements for a type to be considered an allocator.
  ///
  /// The `mtc::allocator` concept defines the necessary requirements for a type to qualify as an allocator.
  /// An allocator type must provide a `value_type` member type, be copy and move constructible, and be equality comparable.
  /// Additionally, it must implement the `allocate` and `deallocate` methods to manage memory allocation and deallocation.
  ///
  /// \tparam T The type to check for allocator requirements.
  ///
  /// \details
  /// The `mtc::allocator` concept ensures that a type meets the following criteria:
  /// - **Member Type**: The type must have a `value_type` member type.
  /// - **Copy Constructible**: The type must be copy constructible.
  /// - **Move Constructible**: The type must be move constructible.
  /// - **Equality Comparable**: The type must be equality comparable.
  /// - **Allocate Method**: The type must provide an `allocate` method that takes a size and returns a pointer to `value_type`.
  /// - **Deallocate Method**: The type must provide a `deallocate` method that takes a pointer to `value_type` and a size.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// static_assert(mtc::allocator<std::allocator<int>>);
  /// \endcode
  ///
  /// In the above example, `std::allocator<int>` is checked to ensure it satisfies the `allocator` concept.
  ///
  /// \note This concept is part of the `[allocator]` module and is used to enforce allocator requirements in template parameters.
  template <class T>
  concept allocator = requires { typename std::remove_cvref_t<T>::value_type; } &&                        // Has a `value_type` member type.
                      std::copy_constructible<T> &&                                                       // Copy constructible.
                      std::move_constructible<T> &&                                                       // Move constructible.
                      std::equality_comparable<T> &&                                                      // Equality comparable.
                      requires(std::remove_cvref_t<T> &a, size_t n, typename std::remove_cvref_t<T>::value_type *p) {
                        { a.allocate(n) } -> std::same_as<typename std::remove_cvref_t<T>::value_type *>; // `allocate` method
                        { a.deallocate(p, n) } -> std::same_as<void>;                                     // `deallocate` method
                      };

  /// \concept mtc::allocator_for
  /// \ingroup allocator
  /// \brief Specifies the requirements for a type to be considered an allocator for a specific value type.
  ///
  /// The `mtc::allocator_for` concept defines the necessary requirements for a type \p T to qualify as an allocator
  /// for a specific value type \p For. This concept ensures that the allocator type \p T meets the general allocator
  /// requirements and that its `value_type` matches the specified type \p For.
  ///
  /// \tparam T The type to check for allocator requirements.
  /// \tparam For The value type that the allocator must support.
  ///
  /// \details
  /// The `mtc::allocator_for` concept ensures that a type \p T meets the following criteria:
  /// - **Allocator Requirements**: The type \p T must satisfy the `mtc::allocator` concept.
  /// - **Value Type Match**: The `value_type` member type of \p T must be the same as \p For.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// static_assert(mtc::allocator_for<std::allocator<int>, int>);
  /// \endcode
  ///
  /// In the above example, `std::allocator<int>` is checked to ensure it satisfies the `mtc::allocator_for` concept
  /// for the value type `int`.
  ///
  /// \note This concept is part of the `[allocator]` module and is used to enforce allocator requirements
  ///       for specific value types in template parameters.
  template <class T, class For>
  concept allocator_for = mtc::allocator<T> && std::same_as<For, typename std::remove_cvref_t<T>::value_type>;


  /// \ingroup allocator
  /// \brief Defines the result type of the `allocate` method for a given allocator type.
  ///
  /// The `mtc::allocate_result_t` type alias is used to determine the return type of the `allocate` method
  /// for a specified allocator type. This type alias is particularly useful in generic programming
  /// where the exact type of the allocator is not known in advance.
  ///
  /// \tparam Allocator The allocator type for which the result type of the `allocate` method is determined.
  ///
  /// \details
  /// The `mtc::allocate_result_t` type alias leverages the `decltype` specifier to deduce the return type
  /// of the `allocate` method for the given allocator type \p Allocator. This ensures that the correct
  /// type is used in contexts where the result of the `allocate` method is required.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// using result_type = mtc::allocate_result_t<std::allocator<int>>;
  /// static_assert(std::is_same_v<result_type, int*>);
  /// \endcode
  ///
  /// In the above example, the `mtc::allocate_result_t` type alias is used to determine that the result type
  /// of the `allocate` method for `std::allocator<int>` is `int*`.
  ///
  /// \note This type alias is part of the `[allocator]` module and is designed to be used in
  ///       high-performance applications where generic allocator interfaces are required.
  template <class Allocator>
    requires mtc::allocator<Allocator>
  using allocate_result_t = decltype(mtc::declval<Allocator>().allocate(mtc::declval<size_t>()));

  namespace cpo {
    struct allocate_t final {
      template <class Allocator>
        requires mtc::allocator<Allocator>
      constexpr auto operator()(Allocator &allocator, size_t n) const noexcept(noexcept(allocator.allocate(n))) -> decltype(allocator.allocate(n)) {
        return allocator.allocate(n);
      }
    };

    struct deallocate_t final {
      template <class Allocator>
        requires mtc::allocator<Allocator>
      constexpr auto operator()(Allocator &allocator, mtc::allocate_result_t<Allocator> allocation, size_t n) const
          noexcept(noexcept(allocator.deallocate(allocation, n))) -> void {
        allocator.deallocate(allocation, n);
      }
    };
  } // namespace cpo

  /// \ingroup allocator
  /// \brief A customization point object for allocating memory using a specified allocator.
  ///
  /// The `mtc::allocate` variable is an instance of the `mtc::cpo::allocate_t` struct, which provides
  /// a customization point for allocating memory through a given allocator. This customization point
  /// ensures that the allocation operation is performed in a consistent and type-safe manner across
  /// different allocator types that satisfy the `mtc::allocator` concept.
  ///
  /// \details
  /// The `mtc::allocate` customization point object invokes the `allocate` method on the provided allocator
  /// instance, passing the specified size as an argument. It leverages the `noexcept` specifier to
  /// propagate the exception specification of the underlying `allocate` method, ensuring that the
  /// noexcept status is preserved.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// std::allocator<int> alloc;
  /// int* ptr = mtc::allocate(alloc, 10);
  /// \endcode
  ///
  /// In the above example, the `mtc::allocate` customization point is used to allocate memory for 10 integers
  /// using the `std::allocator<int>` instance.
  ///
  /// \note This customization point is part of the `[allocator]` module and is designed to be used
  ///       in high-performance applications where custom memory management is required.
  inline constexpr auto allocate = mtc::cpo::allocate_t{};

  /// \ingroup allocator
  /// \brief A customization point object for deallocating memory using a specified allocator.
  ///
  /// The `mtc::deallocate` variable is an instance of the `mtc::cpo::deallocate_t` struct, which provides
  /// a customization point for deallocating memory through a given allocator. This customization point
  /// ensures that the deallocation operation is performed in a consistent and type-safe manner across
  /// different allocator types that satisfy the `mtc::allocator` concept.
  ///
  /// \details
  /// The `mtc::deallocate` customization point object invokes the `deallocate` method on the provided allocator
  /// instance, passing the allocated memory pointer and the size as arguments. It leverages the `noexcept` specifier
  /// to propagate the exception specification of the underlying `deallocate` method, ensuring that the
  /// noexcept status is preserved.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// std::allocator<int> alloc;
  /// int* ptr = alloc.allocate(10);
  /// mtc::deallocate(alloc, ptr, 10);
  /// \endcode
  ///
  /// In the above example, the `mtc::deallocate` customization point is used to deallocate memory for 10 integers
  /// that were previously allocated using the `std::allocator<int>` instance.
  ///
  /// \note This customization point is part of the `[allocator]` module and is designed to be used
  ///       in high-performance applications where custom memory management is required.
  inline constexpr auto deallocate = mtc::cpo::deallocate_t{};

} // namespace mtc

#endif // MTC_ALLOCATOR_HPP
