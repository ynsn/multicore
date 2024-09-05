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
  inline namespace v1 {
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
        requires { typename std::remove_cvref_t<Allocator>::value_type; } &&  // Has a `value_type` member type.
        std::copy_constructible<Allocator> &&                                 // Copy constructible.
        std::move_constructible<Allocator> &&                                 // Move constructible.
        std::equality_comparable<Allocator> &&                                // Equality comparable.
        requires(Allocator &&a, const Allocator &b, Allocator c, size_t n, typename std::remove_cvref_t<Allocator>::value_type *p) {
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

    namespace cpo {
      struct allocate_t {
        template <class Allocator>
          requires allocator<Allocator>
        constexpr auto operator()(Allocator &&allocator, size_t n) const noexcept(noexcept(MTC_FWD(allocator).allocate(n)))
            -> allocate_result_t<Allocator> {
          return MTC_FWD(allocator).allocate(n);
        }
      };

      struct deallocate_t {
        template <class Allocator>
          requires allocator<Allocator>
        constexpr auto operator()(Allocator &&allocator, allocate_result_t<Allocator> allocation, size_t n) const
            noexcept(noexcept(MTC_FWD(allocator).deallocate(allocation, n))) -> void {
          MTC_FWD(allocator).deallocate(allocation, n);
        }
      };
    }  // namespace cpo

    //---------------------------------------------------------------------------------------------------------------------------------------------
    /// \name Allocator Customization Point Objects
    /// \brief Customization point objects providing a means to allocate and deallocate memory using allocators.
    /// @{

    /// \ingroup allocator
    /// \brief The `allocate` customization point object is used to allocate memory using an allocator.
    /// \details The `allocate` customization point object is callable as if it is a free function with the following signature:
    /// \code
    /// constexpr auto allocate(mtc::allocator auto &&allocator, size_t n);
    /// \endcode
    /// \param allocator The allocator to allocate memory with.
    /// \param n The amount of elements to allocate.
    /// \return Memory pointer to the allocated memory.
    /// \note The `allocator` parameter must satisfy the `mtc::allocator` concept.
    /// \see mtc::allocator
    /// \hideinitializer
    inline constexpr auto allocate = cpo::allocate_t{};

    /// \ingroup allocator
    /// \brief The `deallocate` customization point object is used to deallocate memory using an allocator.
    /// \details The `deallocate` customization point object is callable as if it is a free function with the following signature:
    /// \code
    /// constexpr auto deallocate(mtc::allocator auto &&allocator, auto allocation, size_t n);
    /// \endcode
    /// \param allocator The allocator to deallocate memory with.
    /// \param allocation The memory pointer to deallocate. This is the pointer returned by the `allocate` method of the allocator.
    /// \param n The amount of elements to deallocate. This is the same value as passed to the `allocate` method.
    /// \note The `allocator` parameter must satisfy the `mtc::allocator` concept.
    /// \note The `allocation` parameter must be the result of a call to `mtc::allocate`.
    /// \note The `n` parameter must be the same value as passed to the `mtc::allocate` method.
    /// \see mtc::allocator
    /// \hideinitializer
    inline constexpr auto deallocate = cpo::deallocate_t{};

    /// @}
    //---------------------------------------------------------------------------------------------------------------------------------------------

    /// \ingroup allocator
    /// \brief The `with_allocator` type is used to specify an allocator in a parameter pack/argument list.
    /// \tparam Allocator The allocator type.
    template <allocator Allocator>
    struct with_allocator {
      [[no_unique_address]] Allocator allocator;

      template <class U = Allocator>
        requires std::constructible_from<Allocator, U &&>
      // ReSharper disable once CppNonExplicitConvertingConstructor
      constexpr with_allocator(U &&u) noexcept : allocator(MTC_FWD(u)) {}  // NOLINT(*-explicit-constructor)
    };
  } // namespace v1
}  // namespace mtc

#endif  // MTC_ALLOCATOR_HPP
