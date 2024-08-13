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

#ifndef MTC_ENV_HPP
#define MTC_ENV_HPP

#include "concepts.hpp"
#include "utility.hpp"

#include <type_traits>

namespace mtc {
  /**
   * \defgroup env env
   * \brief The `env` module provides facilities for working with queries, queryables and environments.
   */

  template <class T>
  concept queryable = destructible<T>;

  template <class Query, class Queryable, class... Args>
  concept query_for = queryable<Queryable> && requires(const Queryable& obj, Args&&... args) {
    { obj.query(Query{}, MTC_FWD(args)...) } -> queryable;
  };

  template <class Query, class Queryable, class... Args>
  concept nothrow_query_for = mtc::queryable<Queryable> && requires(const Queryable& obj, Args&&... args) {
    { obj.query(Query{}, MTC_FWD(args)...) } noexcept -> queryable;
  };

  template <class Query, class Queryable, class... Args>
    requires query_for<Query, Queryable, Args...>
  using query_for_result_t = decltype(declval<Queryable>().query(Query{}, declval<Args>()...));

  struct forwarding_query_t {
    template <class Query>
    __forceinline constexpr auto operator()(Query q) const noexcept -> bool {
      if constexpr (query_for<forwarding_query_t, Query>) {
        static_assert(noexcept(q.query(*this)), "query(forwarding_query_t) must be noexcept");
        static_assert(same_as<decltype(q.query(*this)), bool>, "query(forwarding_query_t) must return a bool");
        return q.query(*this);
      } else if constexpr (derived_from<Query, forwarding_query_t>) {
        return true;
      } else {
        return false;
      }
    }
  };

  struct query_or_t {
    template <class Query, class Queryable, class Default>
    constexpr auto operator()(Query, Queryable&&, Default&& value) const noexcept(nothrow_constructible_from<Default, Default&&>) -> Default {
      return MTC_FWD(value);
    }

    template <class Query, class Queryable, class Default>
      requires callable<Query, Queryable>
    constexpr auto operator()(Query query, Queryable&& obj, Default&&) const noexcept(nothrow_callable<Query, Queryable>)
        -> call_result_t<Query, Queryable> {
      return MTC_FWD(query)(MTC_FWD(obj));
    }
  };

  inline constexpr auto forwarding_query = forwarding_query_t{};
  inline constexpr auto query_or = query_or_t{};

  template <class Query, class Value>
  struct prop {
    using query_type = Query;
    using value_type = Value;

    [[msvc::no_unique_address]] query_type _query;
    [[msvc::no_unique_address]] value_type _value;

    [[nodiscard]] constexpr auto query(Query) const noexcept { return _value; }
  };

  template <class Query, class Value>
  prop(Query, Value) -> prop<Query, std::unwrap_reference_t<Value>>;

  template <class... Envs>
  struct env;

  template <>
  struct env<> {
    constexpr env() noexcept = default;

    template <class Query>
    constexpr auto query(Query) const noexcept -> void = delete;
  };

  template <class Env, class... Envs>
  struct env<Env, Envs...> {
    [[msvc::no_unique_address]] Env head;
    [[msvc::no_unique_address]] env<Envs...> tail;

    constexpr explicit env(Env h, Envs... t) : head{MTC_MOVE(h)}, tail{MTC_MOVE(t)...} {}

    template <class Query>
    [[nodiscard]] constexpr auto query(Query query) const noexcept -> decltype(auto) {
      if constexpr (requires { head.query(query); }) {
        return head.query(query);
      } else {
        return tail.query(query);
      }
    }
  };

  template <class... Envs>
  env(Envs...) -> env<std::unwrap_reference_t<Envs>...>;

  struct with_t {
    template <class Tag, class Value>
    constexpr auto operator()(Tag tag, Value&& value) const noexcept -> prop<Tag, Value> {
      return prop(tag, MTC_FWD(value));
    }
  };

  inline constexpr auto with = with_t{};
  using empty_env = env<>;

  struct get_env_t {
    template <class T>
      requires requires(const T& obj) {
        { obj.get_env() };
      }
    constexpr auto operator()(const T& obj) const noexcept -> decltype(obj.get_env()) {
      static_assert(mtc::queryable<decltype(obj.get_env())>, "get_env() must return a `queryable` type");
      static_assert(noexcept(obj.get_env()), "get_env() must be noexcept");
      return obj.get_env();
    }

    template <class T>
    constexpr auto operator()(const T& obj) const noexcept -> empty_env {
      return empty_env{};
    }
  };

  inline constexpr auto get_env = get_env_t{};

  template <class T>
  using env_of_t = call_result_t<get_env_t, T>;

  template <class T>
  concept environment_provider = requires(const T& obj) {
    { get_env(obj) } noexcept -> queryable;
  };
}  // namespace mtc

#endif  // MTC_ENV_HPP
