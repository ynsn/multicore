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
#include <multicore/stop_token.hpp>

#include "doctest/doctest.hpp"

TEST_CASE("inplace_stop_token") {
  SUBCASE("unassociated inplace_stop_token cannot issue stop request") {
    const mtc::inplace_stop_token token;
    REQUIRE_FALSE(token.stop_requested());
    REQUIRE_FALSE(token.stop_possible());
  }

  SUBCASE("inplace_stop_token associated with inplace_stop_source can issue stop request") {
    mtc::inplace_stop_source source;
    mtc::inplace_stop_token token;
    REQUIRE_FALSE(source.stop_requested());  // No stop is requested yet
    REQUIRE(source.stop_possible());         // Stop is possible
    REQUIRE_FALSE(token.stop_requested());   // Stop token is not associated with a source, so no stop is requested
    REQUIRE_FALSE(token.stop_possible());    // Stop token is not associated with a source, so stop is not possible

    token = source.get_token();             // Associate the token with the source
    REQUIRE_FALSE(token.stop_requested());  // No stop is requested yet
    REQUIRE(token.stop_possible());         // Stop is possible now

    REQUIRE(source.request_stop());  // Request stop, returns true since stop was not requested yet

    REQUIRE_FALSE(source.request_stop());  // Request stop again, returns false since stop was already requested
    REQUIRE(source.stop_requested());      // Stop is requested
    REQUIRE(source.stop_possible());       // Stop is possible
    REQUIRE(token.stop_requested());       // Token received the stop request
    REQUIRE_FALSE(token.stop_possible());  // Stop is not possible anymore because the token is associated with a source that has requested stop
  }

  SUBCASE("inplace_stop_token can have associated stop classbacks") {
    mtc::inplace_stop_source source;
    mtc::inplace_stop_token token = source.get_token();

    size_t invocations{0ULL};

    mtc::inplace_stop_callback cb1{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb2{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb3{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb4{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb5{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb6{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb7{token, [&]() { ++invocations; }};
    mtc::inplace_stop_callback cb8{token, [&]() { ++invocations; }};

    REQUIRE(source.request_stop());
    REQUIRE_EQ(invocations, 8ULL);
  }

  SUBCASE("inplace_stop_token can have associated stop classbacks") {
    mtc::inplace_stop_source source;
    mtc::inplace_stop_token token = source.get_token();

    size_t invocations{0ULL};

    mtc::inplace_stop_callback cb1{token, [&]() {
                                     ++invocations;
                                   }};

    std::jthread worker1{[&]() {
      mtc::inplace_stop_callback cb2{token, [&]() {
                                       ++invocations;
                                     }};

      REQUIRE(source.request_stop());
      REQUIRE_EQ(invocations, 2ULL);
    }};
  }
}
