// Copyright (c) 2023 - present, Yoram Janssen and Multicore contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
// do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial
//  portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// --- Optional exception to the license ---
//
// As an exception, if, as a result of your compiling your source code, portions of this Software are embedded into a
//  machine-executable object form of such source code, you may redistribute such embedded portions in such
//  object form without including the above copyright and permission notices.

#ifndef MULTICORE_TAG_INVOKE_H
#define MULTICORE_TAG_INVOKE_H

#include "../utility/forward.h"

namespace mtc {
namespace _tag_invoke {
  inline constexpr auto tag_invoke() -> void = delete;

  template <class Tag, class... Args>
  concept tag_invocable = requires(Tag tag, Args&&... args) {
    { tag_invoke(MTC_FWD(tag), MTC_FWD(args)...) };
  };

  template <class Tag, class... Args>
  concept tag_invocable_with = requires(Tag tag, Args&&... args) {
    { tag_invoke(MTC_FWD(tag), MTC_FWD(args)...) };
  };

  template <class Tag, class... Args>
  concept nothrow_tag_invocable = requires(Tag tag, Args&&... args) {
    { tag_invoke(MTC_FWD(tag), MTC_FWD(args)...) } noexcept;
  };

  template <class Tag, class... Args>
  concept nothrow_tag_invocable_with = requires(Tag tag, Args&&... args) {
    { tag_invoke(MTC_FWD(tag), MTC_FWD(args)...) } noexcept;
  };

  template <class Ret, class Tag, class... Args>
  concept tag_invocable_of = requires(Tag tag, Args&&... args) {
    { tag_invoke(MTC_FWD(tag), MTC_FWD(args)...) } -> std::same_as<Ret>;
  };

  template <class Ret, class Tag, class... Args>
  concept nothrow_tag_invocable_of = requires(Tag tag, Args&&... args) {
    { tag_invoke(MTC_FWD(tag), MTC_FWD(args)...) } noexcept -> std::same_as<Ret>;
  };

  template <class Tag, class... Args>
    requires(tag_invocable<Tag, Args...>)
  struct tag_invoke_result : std::type_identity<decltype(tag_invoke(std::declval<Tag>(), std::declval<Args>()...))> {};

  template <class Tag, class... Args>
    requires(tag_invocable<Tag, Args...>)
  using tag_invoke_result_t = typename tag_invoke_result<Tag, Args...>::type;

  struct tag_invoke_t final {
    template <class Tag, class... Args>
      requires(tag_invocable<Tag, Args...>)
    constexpr auto operator()(Tag tag, Args&&... args) const noexcept(nothrow_tag_invocable<Tag, Args...>) {
      return tag_invoke(MTC_FWD(tag), MTC_FWD(args)...);
    }
  };
}  // namespace _tag_invoke

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;

inline constexpr auto tag_invoke = _tag_invoke::tag_invoke_t{};

}  // namespace mtc

#endif  // MULTICORE_TAG_INVOKE_H
