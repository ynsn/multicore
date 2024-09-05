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

#ifndef MTC_REFLECT_HPP
#define MTC_REFLECT_HPP

#include <array>
#include <string_view>

namespace mtc {

  /**
   * \defgroup reflect reflect
   * \brief The `reflect` module provides utilities for reflection.
   */

  namespace detail {
    template <size_t... Is>
    [[nodiscard]] constexpr auto substring_buffer(std::string_view string, std::index_sequence<Is...>) {
      return std::array{string[Is]..., '\0'};
    }

    template <class T>
    [[nodiscard]] constexpr auto type_name_array() noexcept -> decltype(auto) {
      constexpr auto prefix = std::string_view{"type_name_array<"};
      constexpr auto suffix = std::string_view{">(void)"};
      constexpr auto signature = std::string_view{__FUNCSIG__};

      constexpr auto begin = signature.find(prefix) + prefix.size();
      constexpr auto end = signature.rfind(suffix);
      static_assert(begin < end, "Invalid type name");
      constexpr auto function_name = signature.substr(begin, end - begin);
      return substring_buffer(function_name, std::make_index_sequence<function_name.size()>{});
    }

    template <class T>
    struct tn {
      static constexpr auto value = type_name_array<T>();
    };
  }  // namespace detail

  /// \ingroup reflect
  /// \brief The `mtc::name_t` type aliases `const char*` used to represent the name of a type.
  using name_t = const char*;

  /// \ingroup reflect
  /// \brief The `mtc::hash_t` type aliases `uint64_t` used to represent the hash of a type.
  using hash_t = uint64_t;

  /// \ingroup reflect
  /// \brief The `mtc::nameof()` function returns the name of the type `T` as a `const char*`.
  /// \tparam T The type to get the name of.
  /// \return The name of the type `T`.
  template <class T>
  [[nodiscard]] constexpr auto nameof() noexcept -> mtc::name_t {
    constexpr auto& name = mtc::detail::tn<T>::value;
    return name.data();
  }

  /// \ingroup reflect
  /// \brief The `mtc::hashof()` function returns the hash of the type `T` as a `uint64_t`.
  /// \tparam T The type to get the hash of.
  /// \return The hash of the type `T`.
  template <class T>
  [[nodiscard]] constexpr auto hashof() noexcept -> mtc::hash_t {
    constexpr auto name = mtc::nameof<T>();  // Get the name of the type.
    size_t name_len = 0;                     // Calculate the length of the name.
    while (name[name_len] != '\0') {
      ++name_len;
    }

    uint64_t hash = 0xcbf29ce484222325;        // FNV-1a hash.
    constexpr uint64_t prime = 0x100000001b3;  // FNV-1a prime.

    for (int i = 0; i < name_len; ++i) {
      const uint8_t value = name[i];
      hash = hash ^ value;
      hash *= prime;
    }

    return hash;
  }
}  // namespace mtc

#endif  // MTC_REFLECT_HPP
