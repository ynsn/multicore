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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "multicore/detail/any.hpp"
#include "doctest/doctest.hpp"

struct allocator_interface {
  template <class B>
  struct interface : B {
    auto allocate(size_t n) -> void* { return this->template dispatch<0>(n); }
  };

  template <class T>
  using vtable_for = mtc::vtable< T, &T::allocate>;
};

struct dummy_allocator {
  auto allocate(size_t n) -> void* { return nullptr; }
};

TEST_CASE("any is empty after construction") {
  mtc::any<allocator_interface> any_allocator;
  REQUIRE_FALSE(any_allocator.has_value());
}

TEST_CASE("any is not empty after emplacement") {
  mtc::any<allocator_interface> any_allocator;
  REQUIRE_FALSE(any_allocator.has_value());

  auto* emplacement = any_allocator.emplace<dummy_allocator>();
  REQUIRE(emplacement != nullptr);
  REQUIRE(any_allocator.has_value());
}

TEST_CASE("any can reset and check for value") {

}