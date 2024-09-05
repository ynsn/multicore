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

#ifndef MTC_ANY_HPP
#define MTC_ANY_HPP

#include <cmath>

#include "memory.hpp"
#include "utility.hpp"

#include <assert.h>
#include <windows.h>
#include <array>
#include <new>
#include <string_view>

namespace mtc {

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

  template <class T>
  [[nodiscard]] constexpr auto type_name() noexcept -> const char * {
    constexpr auto &name = detail::tn<T>::value;
    return name.data();  // std::string_view{name.data(), name.size()};
  }

  struct type_info {
    const char *name;
    size_t size;
    size_t alignment;
  };

  template <class T>
  [[nodiscard]] constexpr auto typeinfo() noexcept -> const type_info & {
    static constexpr type_info info = {type_name<T>(), sizeof(T), alignof(T)};
    return info;
  }

  // static_assert(std::is_trivial_v<type_info>);

  template <class Derived, class Base, size_t Size, size_t Alignment = alignof(max_align_t)>
  class any_adaptor {
   public:
    friend Derived;

    static constexpr auto size = Size;
    static constexpr auto alignment = Alignment;

    enum builtin_operation : int {
      destructor = 0,
      get_type = 1,
      copy_construct = 2,
      move_construct = 3,
      copy_assign = 4,
      move_assign = 5,
    };

   public:
    constexpr any_adaptor() noexcept = default;
    constexpr any_adaptor(const any_adaptor &other) : vptr{other.vptr} { vptr(storage, copy_construct, other.storage, nullptr); }
    constexpr any_adaptor(any_adaptor &&other) noexcept : vptr{mtc::exchange(other.vptr, empty_vtable)} {
      vptr(storage, move_construct, MTC_MOVE(other).storage, nullptr);
    }
    constexpr ~any_adaptor() noexcept { vptr(storage, destructor, nullptr, nullptr); }

    constexpr auto operator=(const any_adaptor &other) -> any_adaptor & {
      if (this == &other) return *this;
      if (vptr == other.vptr) {
        vptr(storage, copy_assign, other.storage, nullptr);
      } else {
        vptr(storage, destructor, nullptr, nullptr);
        vptr = other.vptr;
        vptr(storage, copy_construct, other.storage, nullptr);
      }
      return *this;
    }

    constexpr auto operator=(any_adaptor &&other) noexcept -> any_adaptor & {
      if (this == &other) return *this;
      if (vptr == other.vptr) {
        vptr(storage, move_assign, MTC_MOVE(other).storage, nullptr);
      } else {
        vptr(storage, destructor, nullptr, nullptr);
        vptr = mtc::exchange(other.vptr, empty_vtable);
        vptr(storage, move_construct, MTC_MOVE(other).storage, nullptr);
      }
      return *this;
    }

   protected:
    template <class ValueType, class... Args>
      requires std::constructible_from<std::decay_t<ValueType>, Args...>
    constexpr auto emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<std::decay_t<ValueType>, Args...>)
        -> std::decay_t<ValueType> & {
      using value_type = std::decay_t<ValueType>;
      static_assert(sizeof(value_type) <= size, "Value type does not fit in storage");
      vptr(storage, destructor, nullptr, nullptr);
      vptr = vtable_for<value_type>;
      new (static_cast<void *>(storage)) value_type(MTC_FWD(args)...);
      return *std::launder(reinterpret_cast<value_type *>(storage));
    }

    constexpr auto reset() -> void {
      vptr(storage, destructor, nullptr, nullptr);
      vptr = empty_vtable;
    }

    [[nodiscard]] constexpr auto type() const noexcept -> type_info {
      type_info type_info_storage;
      vptr(nullptr, get_type, nullptr, reinterpret_cast<unsigned char *>(&type_info_storage));
      return type_info_storage;
    }

   protected:
    constexpr auto dispatch(int call, const void *args, unsigned char *ret)  -> void {
      vptr(storage, call, args, ret);
    }

    constexpr auto dispatch(int call, const void *args, unsigned char *ret) const -> void {
      vptr(storage, call, args, ret);
    }

    using vtable = void(void*, unsigned char *, int, const void *, unsigned char *);
    static constexpr auto empty_vtable(void*, unsigned char *, int, const void *, unsigned char *) noexcept -> void {}

    template <class DecayedValueType>
    static constexpr auto destroy(DecayedValueType *storage) noexcept -> void {
      storage->~DecayedValueType();
    }

    template <class DecayedValueType>
      requires(std::copy_constructible<DecayedValueType>)
    static constexpr auto copy(DecayedValueType *storage, const DecayedValueType &other) noexcept -> void {
      new (storage) DecayedValueType(other);
    }

    template <class DecayedValueType>
    [[noreturn]] static constexpr auto copy(DecayedValueType *, const DecayedValueType &) -> void {
      std::terminate();
    }

    template <class DecayedValueType>
    static constexpr auto vtable_for(unsigned char *storage, int call, const void *args, unsigned char *ret) -> void {
      using type = DecayedValueType;

      static constexpr auto info = type_info{.name = type_name<DecayedValueType>(), .size = sizeof(type), .alignment = alignof(type)};
      auto *storage_pointer = reinterpret_cast<type *>(storage);
      auto *object_pointer = std::launder(storage_pointer);

      switch (call) {
        case builtin_operation::destructor: {  // Invoke destructor
          destroy<type>(object_pointer);
        } break;

        case builtin_operation::get_type: {  // Obtain type information
          auto *type_info_storage_pointer = reinterpret_cast<type_info *>(ret);
          auto *type_info_object_pointer = std::launder(type_info_storage_pointer);
          *type_info_object_pointer = info;
        } break;

        case builtin_operation::copy_construct: {  // Copy construct
          const auto *other_storage_const_pointer = reinterpret_cast<const type *>(args);
          const auto *other_object_const_pointer = std::launder(other_storage_const_pointer);
          copy<type>(object_pointer, *other_object_const_pointer);
        } break;

        case builtin_operation::move_construct: {  // Move construct
          const auto *other_storage_const_pointer = reinterpret_cast<const type *>(args);
          auto *other_object_pointer = std::launder(const_cast<type *>(other_storage_const_pointer));
          new (object_pointer) type(MTC_MOVE(*other_object_pointer));
        } break;

        case builtin_operation::copy_assign: {  // Copy assign
          const auto *other_storage_const_pointer = reinterpret_cast<const type *>(args);
          const auto *other_object_const_pointer = std::launder(other_storage_const_pointer);
          //          *object_pointer = *other_object_const_pointer;
        } break;

        case builtin_operation::move_assign: {  // Move assign
          const auto *other_storage_const_pointer = reinterpret_cast<const type *>(args);
          auto *other_object_pointer = std::launder(const_cast<type *>(other_storage_const_pointer));
          *object_pointer = MTC_MOVE(*other_object_pointer);
        } break;

        default:
          if constexpr (requires { Derived::template dispatch_invoke<type>(storage, call, args, ret); }) {
            Derived::template dispatch_invoke<type>(storage, call, args, ret);
          }
          break;
      }
    }

    vtable *vptr{empty_vtable};
    mutable alignas(alignment) unsigned char storage[size]{};
  };

}  // namespace mtc

#endif  // MTC_ANY_HPP
