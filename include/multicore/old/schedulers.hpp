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

// #include "env.hpp"
#include "any.hpp"
#include "coroutine.hpp"
#include "memory.hpp"

#include <cassert>
#include <iostream>

namespace mtc {

  /**
   * \defgroup schedulers schedulers
   * \brief The `schedulers` module provides facilities for working with schedulers.
   */

  template <class Scheduler>
  concept scheduler = requires { typename std::remove_cvref_t<Scheduler>::schedule_operation; } &&  // `schedule_operation` type alias.
                      requires { typename std::remove_cvref_t<Scheduler>::schedule_operation::scheduler_type; } &&  // `scheduler_type`  alias.
                      mtc::awaitable<typename std::remove_cvref_t<Scheduler>::schedule_operation> &&  // `schedule_operation` is awaitable.
                      std::move_constructible<Scheduler> &&                                           // Move constructible.
                      std::is_nothrow_move_constructible_v<Scheduler> &&                              // Nothrow move constructible.
                      std::equality_comparable<Scheduler> &&                                          // Equality comparable.
                      std::is_nothrow_destructible_v<Scheduler> &&                                    // Nothrow destructible.
                      requires(Scheduler &&sched) {                                                   //
                        { MTC_FWD(sched).schedule() } -> awaiter;                                     // `schedule` method
                      };                                                                              //

  template <class Scheduler>
    requires requires(Scheduler &&scheduler) {
      { MTC_FWD(scheduler).schedule() } -> awaiter;
    }
  using schedule_result_t = decltype(declval<Scheduler>().schedule());

  inline namespace cpo {
    struct schedule_t {
      template <class Scheduler>
        requires requires(Scheduler &&scheduler) {
          { MTC_FWD(scheduler).schedule() } -> awaiter;
        }
      constexpr auto operator()(Scheduler &&scheduler) const noexcept -> schedule_result_t<std::remove_reference_t<Scheduler>> {
        return MTC_FWD(scheduler).schedule();
      }
    };
  }  // namespace cpo

  inline constexpr auto schedule = schedule_t{};

  /// \ingroup schedulers
  /// \brief The `mtc::any_scheduler` class provides a type-safe but type-erased container to store any type that satisfies the `mtc::scheduler`
  /// concept.
  class any_scheduler {
   public:
    struct schedule_operation;
    struct scheduler_type_info {
      std::string_view name;
      uint64_t hash;
      uint64_t size;
      uint64_t align;
    };

    enum class storage_allocation { inline_storage, dynamic_storage };

    static constexpr auto inline_storage_size = sizeof(void *) * 1;
    using inline_storage_type = unsigned char[inline_storage_size];

    constexpr any_scheduler() noexcept = default;
    constexpr any_scheduler(const any_scheduler &other) : vptr(other.vptr) { copy_from(other); }
    constexpr any_scheduler(any_scheduler &&other) noexcept : vptr(mtc::exchange(other.vptr, &empty_vtable)) { move_from(MTC_MOVE(other)); }
    constexpr ~any_scheduler() noexcept { destroy(); }

    template <mtc::scheduler Scheduler>
      requires(!std::same_as<std::decay_t<Scheduler>, any_scheduler>)
    constexpr explicit any_scheduler(Scheduler &&scheduler) noexcept {
      emplace<Scheduler>(MTC_FWD(scheduler));
    }

    auto operator=(const any_scheduler &other) noexcept -> any_scheduler & {
      if (this == &other) return *this;

      if (other.vptr == vptr) {
        copy_assign_from(other);
      } else {
        destroy();
        vptr = other.vptr;
        copy_from(other);
      }

      return *this;
    }

    auto operator=(any_scheduler &&other) noexcept -> any_scheduler & {
      if (this == &other) return *this;

      if (other.vptr == vptr) {
        move_assign_from(MTC_MOVE(other));
      } else {
        destroy();
        vptr = mtc::exchange(other.vptr, &empty_vtable);
        move_from(MTC_MOVE(other));
      }

      return *this;
    }

    /// \brief The `assign(const any_scheduler &)` method copy-assigns this scheduler from another `any_scheduler` object.
    /// \param other The `any_scheduler` object to copy-assign from.
    auto assign(const any_scheduler &other) noexcept -> void;
    /// \brief The `assign(any_scheduler &&)` method move-assigns this scheduler from another `any_scheduler` object.
    /// \param other The `any_scheduler` object to move-assign from.
    auto assign(any_scheduler &&other) noexcept -> void;

    //-------------------------------------------------------------------------------------------------------------------------------------------
    /// \name Modifiers
    /// @{
   public:
    /// \brief The `emplace(auto&&...)` method changes the contained `mtc::scheduler` object to a new object of type
    /// `std::decay_t<ValueType>` constructed from the arguments. This method mandates that the size of the object is less than or equal to the
    /// size of the inline storage.
    /// \tparam ValueType The type of the scheduler to construct.
    /// \tparam Args The types of the arguments to pass to the constructor of `std::decay<ValueType>`.
    /// \param args The arguments to pass to the constructor of `std::decay_t<ValueType>`.
    /// \return A reference to the newly constructed `std::decay_t<ValueType>` object.
    template <class ValueType, class... Args>
      requires std::is_constructible_v<std::decay_t<ValueType>, Args...> &&  //
               mtc::scheduler<std::decay_t<ValueType>>
    constexpr auto emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<std::decay_t<ValueType>, Args...>)
        -> std::decay_t<ValueType> &;

    /// \brief The `emplace(std::initializer_list<?>, auto&&...)` method changes the contained `mtc::scheduler` object to a new object of type
    /// `std::decay_t<ValueType>` constructed from the arguments. This method mandates that the size of the object is less than or equal to the
    /// size of the inline storage.
    /// \tparam ValueType The type of the scheduler to construct.
    /// \tparam U The type of the elements in the initializer list.
    /// \tparam Args The types of the arguments to pass to the constructor of `std::decay<ValueType>`.
    /// \param init_list The initializer list to pass to the constructor of `std::decay_t<ValueType>`.
    /// \param args The arguments to pass to the constructor of `std::decay_t<ValueType>`.
    /// \return A reference to the newly constructed `std::decay_t<ValueType>` object.
    template <class ValueType, class U, class... Args>
      requires std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...> &&  //
               mtc::scheduler<std::decay_t<ValueType>>                                                   //
    constexpr auto emplace(std::initializer_list<U> init_list, Args &&...args) noexcept(
        std::is_nothrow_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...>) -> std::decay_t<ValueType> &;

    /// \brief The `reset()` method destroys the contained scheduler, if any.
    constexpr auto reset() noexcept -> void;

    // constexpr auto swap(any_scheduler &other) noexcept -> void;

    /// @}
    //-------------------------------------------------------------------------------------------------------------------------------------------
    /// \name Observers
    /// @{
   public:
    /// \brief The `has_value()` method checks whether any scheduler is contained in this `any_scheduler` object.
    /// \return `true` if a scheduler is contained, `false` otherwise.
    [[nodiscard]] constexpr auto has_value() const noexcept -> bool { return vptr != &empty_vtable; }

    /// \brief The `empty()` method checks whether no scheduler is contained in this `any_scheduler` object.
    /// \return `true` if no scheduler is contained, `false` otherwise.
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return !has_value(); }

    /// \brief The `type()` method returns the type information of the contained scheduler. If no scheduler is contained, zeroed type information
    /// is returned and the name is set to "<empty>".
    /// \return The type information of the contained scheduler.
    [[nodiscard]] constexpr auto type() const noexcept -> const scheduler_type_info & { return vptr->info; }

    /// @}
   private:
    using dispatcher = void(unsigned char *, int, const void *, unsigned char *);
    struct vtable final {
      scheduler_type_info info;
      dispatcher *dispatch;

      template <class DecayedValueType>
      static auto do_dispatch(unsigned char *storage, const int call, const void *args, unsigned char *ret) -> void {
        using value_type = DecayedValueType;
        auto *object_pointer = reinterpret_cast<value_type *>(storage);
        auto *object = std::launder(object_pointer);

        switch (call) {
          case 0: {  // Destructor
            object->~value_type();
          } break;

          case 1: {  // Copy constructor
            const auto *copied_object_pointer = reinterpret_cast<const value_type *>(args);
            new (object) value_type(*std::launder(copied_object_pointer));
          } break;

          case 2: {  // Move constructor
            const auto *moved_object_const_pointer = reinterpret_cast<const value_type *>(args);
            auto *moved_object_pointer = std::launder(const_cast<value_type *>(moved_object_const_pointer));
            new (object) value_type(MTC_MOVE(*std::launder(moved_object_pointer)));
          } break;

          case 3: {  // Copy assignment
            const auto *copied_object_pointer = reinterpret_cast<const value_type *>(args);
            *object = *copied_object_pointer;
          } break;

          case 4: {  // Move assignment
            const auto *moved_object_const_pointer = reinterpret_cast<const value_type *>(args);
            auto *moved_object_pointer = std::launder(const_cast<value_type *>(moved_object_const_pointer));
            *object = MTC_MOVE(*moved_object_pointer);
          } break;

          case 5: {  // Schedule

          } break;

          default:
            break;
        }
      }
    };

    constexpr auto destroy() noexcept -> void { vptr->dispatch(storage, 0, nullptr, nullptr); }
    constexpr auto copy_from(const any_scheduler &other) noexcept -> void { vptr->dispatch(storage, 1, other.storage, nullptr); }
    constexpr auto move_from(any_scheduler &&other) noexcept -> void { vptr->dispatch(storage, 2, MTC_MOVE(other).storage, nullptr); }
    constexpr auto copy_assign_from(const any_scheduler &other) noexcept -> void { vptr->dispatch(storage, 3, other.storage, nullptr); }
    constexpr auto move_assign_from(any_scheduler &&other) noexcept -> void { vptr->dispatch(storage, 4, MTC_MOVE(other).storage, nullptr); }

    template <class DecayedValueType>
    static constexpr auto make_scheduler_vtable() noexcept -> const vtable * {
      static constexpr vtable inst = vtable{
          .info = {.name = mtc::type_name<DecayedValueType>(),
                   .hash = 0,
                   .size = sizeof(DecayedValueType),
                   .align = alignof(DecayedValueType)},
          .dispatch = vtable::do_dispatch<DecayedValueType>,
      };

      return &inst;
    }

    static constexpr vtable empty_vtable = {.info = {"", 0, 0, 0},
                                            .dispatch = +[](unsigned char *, int, const void *, unsigned char *) noexcept {}};

    const vtable *vptr{&empty_vtable};
    alignas(alignof(max_align_t)) inline_storage_type storage{};
  };

  template <class ValueType, class... Args>
    requires std::is_constructible_v<std::decay_t<ValueType>, Args...> && mtc::scheduler<std::decay_t<ValueType>>
  constexpr auto any_scheduler::emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<std::decay_t<ValueType>, Args...>)
      -> std::decay_t<ValueType> & {
    using value_type = std::decay_t<ValueType>;
    static_assert(sizeof(std::decay_t<ValueType>) <= any_scheduler::inline_storage_size, "The size of the object is too large.");
    destroy();
    vptr = make_scheduler_vtable<value_type>();
    new (static_cast<void *>(storage)) value_type(MTC_FWD(args)...);
    return *std::launder(reinterpret_cast<value_type *>(storage));
  }

  template <class ValueType, class U, class... Args>
    requires std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...> && mtc::scheduler<std::decay_t<ValueType>>
  constexpr auto any_scheduler::emplace(std::initializer_list<U> init_list, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...>) -> std::decay_t<ValueType> & {
    using value_type = std::decay_t<ValueType>;
    destroy();
    vptr = make_scheduler_vtable<value_type>();
    new (static_cast<void *>(storage)) value_type(init_list, MTC_FWD(args)...);
    return *std::launder(reinterpret_cast<value_type *>(storage));
  }

  constexpr auto any_scheduler::reset() noexcept -> void {
    destroy();
    vptr = &empty_vtable;
  }

  /// \ingroup schedulers
  /// \brief The `mtc::any_scheduler` class represents a type-erased but type-safe container for any other type that satisfies the
  /// `mtc::scheduler` concept.
  class any_scheduler2 {
   public:
    static constexpr auto inline_storage_size = sizeof(void *) * 1;
    using inline_storage_type = unsigned char[inline_storage_size];

    enum class call_type {
      destruct,
      copy_assign,
      copy_construct,
      move_assign,
      move_construct,
      enum_begin = destruct,
      enum_end = move_construct
    };

    using dispatcher_type = void(unsigned char *, call_type call, const void *, unsigned char *);

    dispatcher_type *dispatcher{nullptr};

    struct schedule_operation {
      using scheduler_type = any_scheduler2;
      [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
      auto await_suspend(std::coroutine_handle<> handle) noexcept -> std::coroutine_handle<> { return await_suspend_fn(this, handle.address()); }
      auto await_resume() const noexcept -> void {}

      void *pointer{nullptr};
      std::coroutine_handle<> (*await_suspend_fn)(void *op, void *){nullptr};

      alignas(alignof(max_align_t)) inline_storage_type inline_storage{};
    };

    constexpr any_scheduler2() noexcept = default;

    template <scheduler Scheduler>
      requires(!std::same_as<std::decay_t<Scheduler>, any_scheduler2>)
    constexpr explicit(false) any_scheduler2(Scheduler &&scheduler) noexcept {
      emplace<Scheduler>(MTC_FWD(scheduler));
    }
    constexpr any_scheduler2(const any_scheduler2 &other) noexcept { other.box->copy(*this, other); }
    constexpr any_scheduler2(any_scheduler2 &&other) noexcept { other.box->move(*this, MTC_MOVE(other)); }
    constexpr ~any_scheduler2() noexcept { /*reset();*/
    }

    auto operator=(const any_scheduler2 &) noexcept -> any_scheduler2 & = default;
    auto operator=(any_scheduler2 &&) noexcept -> any_scheduler2 & = default;
    auto operator==(const any_scheduler2 &) const noexcept -> bool { return true; }

    auto assign(const any_scheduler2 &other) noexcept -> void {
      if (this != &other) {
        other.box->copy(*this, other);
      }
    }

    auto assign(any_scheduler2 &&other) noexcept -> void {
      if (this != &other) {
        other.box->move(*this, MTC_MOVE(other));
      }
    }

    template <mtc::scheduler Scheduler, class... Args>
      requires std::constructible_from<std::decay_t<Scheduler>, Args...>
    constexpr auto emplace(Args &&...args) -> std::decay_t<Scheduler> & {
      using scheduler_type = std::decay_t<Scheduler>;
      reset();

      box = make_any_scheduler_box<scheduler_type>();
      mtc::construct_at<scheduler_type>(std::launder(reinterpret_cast<scheduler_type *>(mtc::addressof(storage))), MTC_FWD(args)...);
      return *std::launder(reinterpret_cast<scheduler_type *>(addressof(storage)));
    }

    template <class T>
    constexpr auto get() const & noexcept -> const T & {
      return *std::launder(reinterpret_cast<const T *>(addressof(storage)));
    }

    template <class T>
    constexpr auto get() && noexcept -> T && {
      return MTC_MOVE(*std::launder(reinterpret_cast<T *>(addressof(storage))));
    }

    /// \brief The `reset` method destroys the contained scheduler if any.
    /// \note This method is a no-op if the `any_scheduler` object does not contain a scheduler.
    constexpr auto reset() noexcept -> void {
      if (box != nullptr) {
        box->destructor(*this);
        // box = nullptr;
      }
    }

    constexpr auto swap(any_scheduler2 &other) noexcept -> void = delete;

    [[nodiscard]] auto schedule() noexcept -> schedule_operation {
      schedule_operation op;
      box->schedule_fn(*this, &op);
      return op;
    }
    [[nodiscard]] auto name() const noexcept -> std::string_view { return box->string; }

   private:
    struct any_scheduler_box {
      std::string_view string{};
      size_t type_hash{};
      size_t size{};
      size_t align{};
      void (*destructor)(any_scheduler2 &) noexcept {nullptr};
      void (*copy)(any_scheduler2 &, const any_scheduler2 &) noexcept {nullptr};
      void (*copy_assign)(any_scheduler2 &, const any_scheduler2 &) noexcept {nullptr};
      void (*move)(any_scheduler2 &, any_scheduler2 &&) noexcept {nullptr};
      void (*schedule_fn)(any_scheduler2 &, schedule_operation *){nullptr};
    };

    any_scheduler_box *box{nullptr};
    alignas(alignof(max_align_t)) inline_storage_type storage;

    template <class Scheduler>
    static constexpr auto make_any_scheduler_box() noexcept -> any_scheduler_box * {
      using scheduler_type = std::decay_t<Scheduler>;
      constexpr auto n = type_name<scheduler_type>();
      const char *data = (char *)n.data();
      uint64_t hash = 0xcbf29ce484222325;
      constexpr uint64_t prime = 0x100000001b3;

      for (int i = 0; i < n.length(); ++i) {
        uint8_t value = data[i];
        hash = hash ^ value;
        hash *= prime;
      }
      static any_scheduler_box info = {
          .string = n,
          .type_hash = hash,
          .size = sizeof(Scheduler),
          .align = alignof(Scheduler),
          .destructor =
              +[](any_scheduler2 &self) noexcept {
                mtc::destroy_at(std::launder(reinterpret_cast<scheduler_type *>(mtc::addressof(self.storage))));
              },

          .copy =
              +[](any_scheduler2 &self, const any_scheduler2 &other) noexcept {
                self.emplace<scheduler_type>(other.get<scheduler_type>());
                // mtc::construct_at<scheduler_type>(std::launder(reinterpret_cast<scheduler_type*>(mtc::addressof(self.storage))),
                // MTC_FWD(args)...);;
              },

          .copy_assign = +[](any_scheduler2 &self, const any_scheduler2 &other) noexcept { other.box->copy(self, other); },

          .move = +[](any_scheduler2 &self,
                      any_scheduler2 &&other) noexcept { self.emplace<scheduler_type>(MTC_MOVE(other).get<scheduler_type>()); },

          .schedule_fn =
              +[](any_scheduler2 &self, schedule_operation *op) {
                auto *scheduler = std::launder(reinterpret_cast<scheduler_type *>(mtc::addressof(self.storage)));
                auto res = scheduler->schedule();
                op->await_suspend_fn = +[](void *self2, void *h) -> std::coroutine_handle<> {
                  auto *sch = std::launder(static_cast<scheduler_type *>(self2));
                  // if (h == nullptr || sch == nullptr) return std::noop_coroutine();
                  return sch->schedule().await_suspend(std::coroutine_handle<>::from_address(h));
                };
                new (&op->inline_storage) schedule_result_t<scheduler_type>{res};
                op->pointer = &op->inline_storage;
              },
      };
      return &info;
    }
  };

  using any_scheduler_ref = const mtc::any_scheduler2 &;

  /// \ingroup schedulers
  /// \brief The `mtc::inline_scheduler` class models a `mtc::scheduler` that runs tasks with a regular function call in the current execution
  /// context.
  struct inline_scheduler {
    struct schedule_operation {
      using scheduler_type = inline_scheduler;
      auto await_ready() const noexcept -> bool { return false; }
      template <class P>
      auto await_suspend(std::coroutine_handle<P> handle) const noexcept -> std::coroutine_handle<> {
        printf("running inline\n");
        // handle.resume();
        return handle;
      }
      auto await_resume() const noexcept -> void {}
    };

    [[nodiscard]] auto schedule() const noexcept -> schedule_operation { return {}; }
    constexpr auto operator==(const inline_scheduler &) const noexcept -> bool = default;
  };

  struct trampoline_scheduler {
    struct schedule_operation {
      constexpr auto await_ready() const noexcept -> bool { return false; }
      auto await_suspend(std::coroutine_handle<> handle) const noexcept -> void {
        // handle.resume();
      }
    };
  };

}  // namespace mtc

#endif  // MTC_SCHEDULERS_HPP
