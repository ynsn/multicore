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

#ifndef MTC_TYPE_INFO_HPP
#define MTC_TYPE_INFO_HPP

#include <string_view>

namespace mtc {
  /**
   * \defgroup type_info type_info
   * \ingroup utility
   * \brief The `type_info` module provides utilities for working with type information.
   */

  namespace detail {
    template <size_t Length>
    struct intermediate_name_array {
      char data[Length];
    };

    template <size_t... Is>
    [[nodiscard]] constexpr auto intermediate_array(std::string_view s, std::index_sequence<Is...>) noexcept {
      return mtc::detail::intermediate_name_array<sizeof...(Is) + 1>{s[Is]..., '\0'};
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

      return intermediate_array(function_name, std::make_index_sequence<function_name.size()>{});
    }

    template <class T>
    struct holder {
      static constexpr auto value = mtc::detail::type_name_array<T>();
    };
  } // namespace detail

  /// \ingroup type_info
  /// \brief Retrieves the name of the specified type \p T as a `const char*`.
  ///
  /// The `mtc::nameof` function is a compile-time utility that returns the name of the type \p T.
  /// This function leverages compile-time string manipulation to extract the type name from
  /// the function signature, ensuring that the name is available as a constant expression.
  ///
  /// \tparam T The type whose name is to be retrieved.
  /// \return A `const char*` representing the name of the type \p T.
  ///
  /// \details
  /// The `mtc::nameof` function is particularly useful in scenarios where type information is required
  /// at compile time, such as in metaprogramming, logging, or debugging. By providing the type name
  /// as a constant expression, this function enables efficient and type-safe retrieval of type names
  /// without incurring runtime overhead.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// std::cout << "Type name: " << mtc::nameof<int>() << std::endl; // Outputs: Type name: int
  ///
  /// struct test {};
  /// std::cout << "Type name: " << mtc::nameof<test>() << std::endl; // Outputs: Type name: struct test
  /// \endcode
  ///
  /// \note This function is part of the `[mtc.utility.type_info]` module and is designed to be used in
  ///       high-performance applications where compile-time type information is essential.
  template <class T>
  [[nodiscard]] constexpr auto nameof() noexcept -> const char * {
    constexpr auto &name = mtc::detail::holder<T>::value;
    return name.data;
  }

  /// \ingroup type_info
  /// \brief Computes the hash of the specified type \p T at compile time.
  ///
  /// The `mtc::hashof` function is a constexpr utility that calculates a unique hash for the type \p T.
  /// This function uses the FNV-1a hash algorithm to generate a 64-bit hash value based on the
  /// compile-time name of the type. The resulting hash can be used for various purposes, such as
  /// type identification, type-based dispatch, or compile-time type information retrieval.
  ///
  /// \tparam T The type whose hash is to be computed.
  /// \return A `uint64_t` representing the hash of the type \p T.
  ///
  /// \details
  /// The `mtc::hashof` function iterates over the characters of the type name obtained from the `nameof`
  /// function and applies the FNV-1a hash algorithm. This ensures that the hash value is unique
  /// for different types, providing a reliable mechanism for type identification at compile time.
  ///
  /// Example usage:
  /// \code{.cpp}
  /// constexpr auto int_hash = mtc::hashof<int>();
  /// constexpr auto string_hash = mtc::hashof<std::string>();
  /// std::cout << "Hash of int: " << int_hash << std::endl;
  /// std::cout << "Hash of std::string: " << string_hash << std::endl;
  /// \endcode
  ///
  /// In the above example, the `hashof` function is used to compute the hash values for the `int`
  /// and `std::string` types, which are then printed to the console.
  ///
  /// \note This function is part of the `[mtc.utility.type_info]` module and is designed to be used in
  ///       high-performance applications where compile-time type information is essential.
  template <class T>
  [[nodiscard]] constexpr auto hashof() noexcept -> uint64_t {
    constexpr auto name = mtc::nameof<T>();
    size_t name_len = 0U;
    while (name[name_len] != '\0') ++name_len;

    uint64_t hash = 0xcbf29ce484222325ULL;
    constexpr uint64_t prime = 0x100000001b3ULL;
    for (size_t i = 0; i < name_len; ++i) {
      const uint8_t value = name[i];
      hash = hash ^ value;
      hash *= prime;
    }

    return hash;
  }

  struct type_info {
    const char *name;
    uint64_t hash;
    size_t size;
    size_t alignment;
  };

  namespace detail {
    template <class T>
    struct info_holder {
      static constexpr auto value = mtc::type_info{.name = mtc::nameof<T>(), .hash = mtc::hashof<T>(), .size = sizeof(T), .alignment = alignof(T)};
    };

    template <>
    struct info_holder<void> {
      static constexpr auto value = mtc::type_info{.name = mtc::nameof<void>(), .hash = mtc::hashof<void>(), .size = 0, .alignment = 0};
    };
  } // namespace detail

  template <class T>
  [[nodiscard]] constexpr auto type_id() noexcept -> mtc::type_info {
    constexpr auto &info = mtc::detail::info_holder<T>::value;
    return info;
  }

  template <class T>
  [[nodiscard]] constexpr auto type_id_ptr() noexcept -> const mtc::type_info * {
    static constexpr auto &info = mtc::detail::info_holder<T>::value;
    return &info;
  }
} // namespace mtc

#endif // MTC_TYPE_INFO_HPP
