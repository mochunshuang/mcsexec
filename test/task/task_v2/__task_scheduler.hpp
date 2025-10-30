#pragma once

#include "../../test_base_head.hpp"
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "./__detail/__any_storage.hpp"

namespace mcs::execution::task_v2
{

    // [exec.task.scheduler]
    class task_scheduler // NOLINT
    {
        using scheduler_storage_type =
            __detail::any_storage<1 * sizeof(void *), alignof(std::max_align_t)>;
        using sender_storage_type =
            __detail::any_storage<1 * sizeof(void *), alignof(std::max_align_t)>;
        using operation_storage_type = // NOLINTNEXTLINE
            __detail::any_storage<8 * sizeof(void *), alignof(std::max_align_t)>;
        using receiver_storage_type =
            __detail::any_storage<1 * sizeof(void *), alignof(std::max_align_t)>;

        struct receiver_any // NOLINTBEGIN
        {
            using receiver_concept = receiver_t;
            struct vtable_t
            {
                void (*set_value)(void *recv_storage) noexcept;
                void (*set_error_ec)(void *recv_storage, std::error_code &&) noexcept;
                void (*set_error_ptr)(void *recv_storage, std::exception_ptr &&) noexcept;
                void (*set_stopped)(void *recv_storage) noexcept;
            };
            const vtable_t *vtable_;
            receiver_storage_type recv_storage_;

            template <typename R, typename Allocator = std::allocator<std::byte>>
            constexpr explicit receiver_any(R &&recv, Allocator alloc = Allocator{})
                : vtable_{create_vtable<std::decay_t<R>>()},
                  recv_storage_{std::forward<R>(recv), std::forward<Allocator>(alloc)}
            {
            }

            constexpr void set_value() && noexcept
            {
                vtable_->set_value(&recv_storage_);
            }

            constexpr void set_error(std::error_code &&ec) && noexcept
            {
                vtable_->set_error_ec(&recv_storage_, std::move(ec));
            }

            constexpr void set_error(std::exception_ptr &&e) && noexcept
            {
                vtable_->set_error_ptr(&recv_storage_, std::move(e));
            }

            constexpr void set_stopped() && noexcept
            {
                vtable_->set_stopped(&recv_storage_);
            }

          private:
            template <typename R>
            constexpr static const vtable_t *create_vtable() noexcept
            {
                const static vtable_t vt = {
                    .set_value = &set_value_impl<R>,
                    .set_error_ec = &set_error_impl<R, std::error_code>,
                    .set_error_ptr = &set_error_impl<R, std::exception_ptr>,
                    .set_stopped = &set_stopped_impl<R>};
                return &vt;
            }

            template <typename R>
            constexpr static void set_value_impl(void *recv_storage) noexcept
            {
                R *recv = receiver_storage_type::get_pointer<R>(recv_storage);
                recv->set_value();
            }

            template <typename R, typename E>
            constexpr static void set_error_impl(void *recv_storage, E &&e) noexcept
            {
                R *recv = receiver_storage_type::get_pointer<R>(recv_storage);
                recv->set_error(std::move(e));
            }

            template <typename R>
            constexpr static void set_stopped_impl(void *recv_storage) noexcept
            {
                R *recv = receiver_storage_type::get_pointer<R>(recv_storage);
                recv->set_stopped();
            }
        }; // NOLINTEND
        static_assert(recv::receiver<receiver_any>);

        struct operation_any // NOLINTBEGIN
        {
            using operation_state_concept = operation_state_t;
            struct vtable_t
            {
                void (*start)(void *op_storage) noexcept;
            };

            const vtable_t *op_vtable_;
            operation_storage_type op_storage_;

            template <typename Operation, typename Allocator>
            constexpr operation_any(Operation &&op, Allocator &&alloc)
                : op_vtable_{create_vtable<std::decay_t<Operation>>()},
                  op_storage_{std::forward<Operation>(op), std::forward<Allocator>(alloc)}
            {
            }

            constexpr void start() & noexcept
            {
                op_vtable_->start(&op_storage_);
            }

          private:
            template <typename O>
            constexpr static vtable_t *create_vtable()
            {
                static vtable_t vt = {.start = &start_impl<O>};
                return &vt;
            }

            template <typename O>
            constexpr static void start_impl(void *op_storage) noexcept
            {
                O *op = operation_storage_type::get_pointer<O>(op_storage);
                op->start();
            }
        }; // NOLINTEND
        static_assert(opstate::operation_state<operation_any>);

        template <queryable RecvEnv, opstate::operation_state Op>
        struct state // NOLINTBEGIN
        {
            using operation_state_concept = operation_state_t;
            constexpr state() = delete;
            constexpr state(const state &) = delete;
            constexpr state(state &&) = delete;
            constexpr state &operator=(const state &) = delete;
            constexpr state &operator=(state &&) = delete;
            constexpr ~state() noexcept = default;

            constexpr state(RecvEnv &&r, Op &&op) noexcept
                : env(std::move(r)), o{std::move(op)}
            {
            }

            constexpr void start() & noexcept
            {
                opstate::start(o);
            }

            [[nodiscard]] constexpr auto get_env() const noexcept // NOLINT
            {
                return env;
            }

            RecvEnv env;
            Op o;
        }; // NOLINTEND

        struct sender // exposition only // NOLINTBEGIN
        {
            using sender_concept = sender_t;
            using completion_signatures = completion_signatures<
                recv::set_value_t(), recv::set_error_t(std::error_code),
                recv::set_error_t(std::exception_ptr), recv::set_stopped_t()>;

            struct vtable_t
            {
                operation_any (*connect)(void *sndr_storage, void *recv) noexcept;
            };
            const vtable_t *sndr_vtable_;
            sender_storage_type sndr_;
            task_scheduler *sch_;

            template <typename Sndr, typename Allocator>
            constexpr sender(Sndr &&sndr, Allocator &&alloc, task_scheduler *sch)
                : sndr_vtable_{create_vtable<std::decay_t<Sndr>,
                                             std::decay_t<Allocator>>()},
                  sndr_{std::forward<Sndr>(sndr), std::forward<Allocator>(alloc)},
                  sch_{sch}
            {
            }

            template <recv::receiver R>
            constexpr auto connect(R &&r) noexcept
            {
                auto env = r.get_env();
                receiver_any recv_any{std::forward<R>(r)};
                return state{std::move(env), sndr_vtable_->connect(&sndr_, &recv_any)};
            }

            struct env
            {
                const sender *sndr_;
                [[nodiscard]] constexpr auto query(
                    queries::get_completion_scheduler_t<set_value_t> /*unused*/)
                    const noexcept
                {
                    return task_scheduler{*(sndr_->sch_)};
                }
            };

            [[nodiscard]] constexpr auto get_env() const noexcept -> env
            {
                return {this};
            }

          private:
            template <typename Sndr, typename Allocator>
            constexpr static vtable_t *create_vtable()
            {
                static vtable_t vt = {.connect = &connect_impl<Sndr, Allocator>};
                return &vt;
            }

            template <typename Sndr, typename Allocator>
            constexpr static operation_any connect_impl(void *sndr_storage,
                                                        void *recv) noexcept
            {
                auto *sndr = sender_storage_type::get_pointer<Sndr>(sndr_storage);
                auto *alloc = sender_storage_type::get_allocator<Allocator>(sndr_storage);
                auto *r = static_cast<receiver_any *>(recv);
                return {sndr->connect(std::move(*r)), *alloc};
            }
        }; // NOLINTEND
        static_assert(snd::sender<sender>);

      public:
        using scheduler_concept = scheduler_t;

        struct vtable_t
        {
            sender (*schedule)(void *sched_storage, task_scheduler *sch);
        };

        template <class Sch, class Allocator = std::allocator<std::byte>>
            requires(!std::same_as<task_scheduler, std::remove_cvref_t<Sch>>) &&
                        scheduler<Sch>
        explicit task_scheduler(Sch &&sch, Allocator alloc = {})
            : vtable_(create_vtable<std::decay_t<Sch>, std::decay_t<Allocator>>()),
              sch_(std::forward<Sch>(sch), std::forward<Allocator>(alloc))
        {
        }

        constexpr sender schedule()
        {
            return vtable_->schedule(&sch_, this);
        }

        friend bool operator==(const task_scheduler &lhs,
                               const task_scheduler &rhs) noexcept
        {
            if (lhs.vtable_ != rhs.vtable_)
                return false;
            return lhs.sch_ == rhs.sch_;
        }

        template <class Sch>
            requires(!std::same_as<task_scheduler, Sch>) && scheduler<Sch>
        friend bool operator==(const task_scheduler &lhs, const Sch &rhs) noexcept
        {
            using DS = std::decay_t<Sch>;

            if (lhs.sch_.stored_type() == typeid(void) ||
                lhs.sch_.stored_type() != typeid(DS))
                return false;
            const auto *stored = scheduler_storage_type::get_pointer<DS>(&lhs.sch_);
            static_assert(
                requires { *stored == rhs; },
                "Sch must implement operator== for comparison");
            return *stored == rhs;
        }

      private:
        // NOLINTBEGIN
        template <class Sch, typename Allocator>
        constexpr static const vtable_t *create_vtable()
        {
            static const vtable_t vt = {.schedule = &schedule_impl<Sch, Allocator>};
            return &vt;
        }

        template <class Sch, typename Allocator>
        constexpr static sender schedule_impl(void *sched_storage, task_scheduler *sch)
        {
            auto *sched = scheduler_storage_type::get_pointer<Sch>(sched_storage);
            auto *alloc = scheduler_storage_type::get_allocator<Allocator>(sched_storage);
            return {sched->schedule(), *alloc, sch};
        }
        // NOLINTEND

        const vtable_t *vtable_;
        scheduler_storage_type sch_; // exposition only
    };

    static_assert(sched::scheduler<task_scheduler>);

}; // namespace mcs::execution::task_v2