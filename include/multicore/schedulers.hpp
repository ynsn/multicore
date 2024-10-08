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

#ifndef MTC_SCHEDULERS_HPP
#define MTC_SCHEDULERS_HPP

namespace mtc {

  namespace cpo {
    struct schedule_t {
      template <class Scheduler>
        requires requires(Scheduler &scheduler) {
          { scheduler.schedule() };
        }
      constexpr auto operator()(Scheduler &scheduler) const noexcept(noexcept(scheduler.schedule())) -> decltype(scheduler.schedule()) {
        return scheduler.schedule();
      }
    };
  }  // namespace cpo

  inline constexpr auto schedule = mtc::cpo::schedule_t{};

  template <class Scheduler>
  concept scheduler = requires(Scheduler &scheduler) {
    { schedule(scheduler) };
  };
}  // namespace mtc

#endif  // MTC_SCHEDULERS_HPP