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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.hpp"

#include <multicore/allocator.hpp>

namespace mtc::tests {
  template <class T, bool Movable, bool Copyable>
  struct mock_allocator {
    using value_type = T;
    mock_allocator() = default;
    mock_allocator(const mock_allocator&)
      requires Copyable
    = default;
    mock_allocator(const mock_allocator&)
      requires !Copyable
    = delete;
    mock_allocator(mock_allocator&&)
      requires Movable
    = default;
    mock_allocator(mock_allocator&&)
      requires !Movable
    = delete;

    constexpr auto operator==(const mock_allocator&) const noexcept -> bool { return true; }

    auto allocate(size_t n) -> T* { return nullptr; }
    auto deallocate(T* pointer, size_t n) -> void { return; }
  };
}  // namespace mtc::tests

TEST_SUITE("allocator") {
  TEST_CASE("concept allocator") {
    REQUIRE(mtc::allocator<mtc::tests::mock_allocator<int, true, true>>);
    REQUIRE_FALSE(mtc::allocator<mtc::tests::mock_allocator<int, false, true>>);
    REQUIRE_FALSE(mtc::allocator<mtc::tests::mock_allocator<int, true, false>>);
    REQUIRE_FALSE(mtc::allocator<mtc::tests::mock_allocator<int, false, false>>);

    REQUIRE(mtc::allocator<mtc::tests::mock_allocator<void, true, true>>);
    REQUIRE_FALSE(mtc::allocator<mtc::tests::mock_allocator<void, false, true>>);
    REQUIRE_FALSE(mtc::allocator<mtc::tests::mock_allocator<void, true, false>>);
    REQUIRE_FALSE(mtc::allocator<mtc::tests::mock_allocator<void, false, false>>);

    REQUIRE_FALSE(mtc::allocator<std::string>);
    REQUIRE_FALSE(mtc::allocator<std::vector<int>>);
    REQUIRE(mtc::allocator<std::string::allocator_type>);
    REQUIRE(mtc::allocator<std::vector<int>::allocator_type>);
  }

  TEST_CASE("concept allocator_for") {
    REQUIRE(mtc::allocator_for<mtc::tests::mock_allocator<int, true, true>, int>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<int, true, true>, float>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<int, false, true>, int>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<int, true, false>, int>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<int, false, false>, int>);

    REQUIRE(mtc::allocator_for<mtc::tests::mock_allocator<void, true, true>, void>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<void, true, true>, char>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<void, false, true>, void>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<void, true, false>, void>);
    REQUIRE_FALSE(mtc::allocator_for<mtc::tests::mock_allocator<void, false, false>, void>);

    REQUIRE_FALSE(mtc::allocator_for<std::string, char>);
    REQUIRE_FALSE(mtc::allocator_for<std::vector<int>, int>);
    REQUIRE(mtc::allocator_for<std::string::allocator_type, char>);
    REQUIRE(mtc::allocator_for<std::vector<int>::allocator_type, int>);
  }

  TEST_CASE("allocator_result_t<T>") {
    REQUIRE(std::same_as<mtc::allocate_result_t<std::allocator<int>>, int*>);
    REQUIRE_FALSE(std::same_as<mtc::allocate_result_t<std::allocator<int>>, void*>);
  }

  TEST_CASE("allocate/deallocate") {
    std::allocator<int> int_alloc;
    CAPTURE(int_alloc);
    auto int_ptr = mtc::allocate(int_alloc, 1);
    CAPTURE(int_ptr);
    CHECK(std::same_as<decltype(int_ptr), int*>);
    CHECK(std::same_as<decltype(int_ptr), mtc::allocate_result_t<decltype(int_alloc)>>);
    CHECK(std::same_as<decltype(int_ptr), typename decltype(int_alloc)::value_type*>);
    if (int_ptr) {
      mtc::deallocate(int_alloc, int_ptr, 1);
    } else {
      FAIL("Failed to allocate memory");
    }
  }
}
