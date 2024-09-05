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

#ifndef MTC_MEMORY_HPP
#define MTC_MEMORY_HPP

#include <concepts>
#include <new>

namespace mtc {

  template <class T>
  [[nodiscard]] constexpr auto addressof(T& value) noexcept -> T* {
    return __builtin_addressof(value);
  }

  template <class T>
  [[nodiscard]] constexpr auto addressof(const T&&) -> const T* = delete;

  template <class T, class... Args>
  constexpr auto construct_at(T* memory, Args&&... args) -> T* {
    return ::new (static_cast<void*>(memory)) T(MTC_FWD(args)...);
  }

  template <class T>
    requires std::is_array_v<T>
  constexpr auto destroy_at(T* pointer) -> void {
    for (const auto& element : *pointer) (destroy_at)(addressof(element));
  }

  template <class T>
  constexpr auto destroy_at(T* pointer) noexcept(noexcept(pointer->~T())) -> void {
    pointer->~T();
  }

  enum class byte : unsigned char {};

  template <class T>
  union uninitialized {
    // intptr_t _;
    T storage;

    constexpr uninitialized() noexcept
      requires std::is_trivially_default_constructible_v<T>
    = default;
    constexpr uninitialized() noexcept(std::is_nothrow_default_constructible_v<T>)
      requires(!std::is_trivially_default_constructible_v<T>)
    {}

    constexpr uninitialized(const uninitialized&) noexcept
      requires std::is_trivially_copy_constructible_v<T>
    = default;
    constexpr uninitialized(const uninitialized& ) noexcept(std::is_nothrow_copy_constructible_v<T>)
      requires(!std::is_trivially_copy_constructible_v<T>)
    {}

    constexpr ~uninitialized() noexcept {}
  };

  template <size_t Size, size_t Alignment = alignof(max_align_t)>
  struct uninitialized_block {
    static constexpr auto size = Size;
    static constexpr auto alignment = Alignment;
    using type = unsigned char[size];

    alignas(Alignment) type storage;

    template <class T>
    [[nodiscard]] constexpr auto as() & noexcept -> std::remove_reference_t<T>* {
      return std::launder(reinterpret_cast<T*>(storage));
    }
  };

  template <class To, class From>
    requires(sizeof(To) == sizeof(From)) && std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From>
  constexpr auto bit_cast(const From& from) noexcept -> To {
    alignas(To) std::byte storage[sizeof(To)];
    const auto pointer = memcpy(storage, mtc::addressof(from), sizeof(To));
    return *reinterpret_cast<const To*>(pointer);
  }

  template <class T>
  constexpr auto start_lifetime_as(const void* ptr) noexcept -> const T* {
    const auto mut_ptr = const_cast<void*>(ptr);
    const auto overlay = new (mut_ptr) std::byte[sizeof(T)];
    const auto live_ptr = reinterpret_cast<const T*>(overlay);
    (void)*live_ptr;
    return live_ptr;
  }

  template <class T>
  constexpr auto start_lifetime_as(void* ptr) noexcept -> T* {
    const auto mut_ptr = ptr;
    const auto overlay = new (mut_ptr) std::byte[sizeof(T)];
    const auto live_ptr = reinterpret_cast<T*>(overlay);
    (void)*live_ptr;
    return live_ptr;
  }

  template <class T>
  class manual_lifetime {
   public:
    constexpr manual_lifetime() noexcept : storage{} {}  // NOLINT;
    constexpr manual_lifetime(const manual_lifetime&) = delete;
    constexpr manual_lifetime(manual_lifetime&&) noexcept = delete;
    constexpr ~manual_lifetime() noexcept = default;

    constexpr auto operator=(const manual_lifetime&) -> manual_lifetime& = delete;
    constexpr auto operator=(manual_lifetime&&) -> manual_lifetime& = delete;

    template <class... Args>
      requires std::constructible_from<T, Args...>
    constexpr auto construct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> void {
      mtc::construct_at<Args...>(mtc::addressof(storage.value), MTC_FWD(args)...);
    }

    constexpr auto destroy() noexcept -> void { mtc::destroy_at(mtc::addressof(storage.value)); }

    [[nodiscard]] constexpr auto get() & noexcept -> T& { return storage.value; }
    [[nodiscard]] constexpr auto get() const& noexcept -> const T& { return storage.value; }
    [[nodiscard]] constexpr auto get() && noexcept -> T&& { return static_cast<T&&>(storage.value); }
    [[nodiscard]] constexpr auto get() const&& noexcept -> const T&& { return static_cast<const T&&>(storage.value); }

   private:
    union value_storage {
      constexpr value_storage() noexcept {}
      constexpr ~value_storage() noexcept {}

      intptr_t _{-1};
      T value;
    } storage{};
  };
}  // namespace mtc

#endif  // MTC_MEMORY_HPP
