#pragma once

#include "../../test_base_head.hpp"
#include <algorithm>
#include <utility>

namespace mcs::execution::task_v2
{
    namespace __detail
    {
        struct schedule_wraper
        {
            using schedule_callback = int;
        };
    }; // namespace __detail
    // [exec.task.scheduler]
    class task_scheduler // NOLINT
    {

        struct sender // exposition only // NOLINT
        {

            using scheduler_concept = scheduler_t;
            using completion_signatures = completion_signatures<
                recv::set_value_t(), recv::set_error_t(std::error_code),
                recv::set_error_t(std::exception_ptr), recv::set_stopped_t()>;

            template <recv::receiver O>
            state<O> connect(O &&rcvr)
            {
            }

            struct env
            {

                [[nodiscard]] constexpr auto query( // NOLINT
                    queries::get_completion_scheduler_t<set_value_t> /*unused*/)
                    const noexcept
                {
                    // return task_scheduler{};
                }
            };

            // NOLINTNEXTLINE
            [[nodiscard]] constexpr auto get_env() const noexcept -> env
            {
                return {};
            }
        };
        template <opstate::operation_state O>
        struct state // exposition only // NOLINT
        {
            O o; // NOLINT

            using operation_state_concept = operation_state_t;
            void start() & noexcept
            {
                opstate::start(o);
            }
        };

      public:
        using scheduler_concept = scheduler_t;

        template <class Sch>
            requires(!std::same_as<task_scheduler, std::remove_cvref_t<Sch>>) &&
                    scheduler<Sch>
        constexpr explicit task_scheduler(Sch sch) noexcept {};

        sender schedule();

        friend bool operator==(const task_scheduler &lhs,
                               const task_scheduler &rhs) noexcept = default;
        template <class Sch>
            requires(!std::same_as<task_scheduler, Sch>) && scheduler<Sch>
        friend bool operator==(const task_scheduler &lhs, const Sch &rhs) noexcept;

      private:
        // shared_ptr<void> sch_; // exposition only
    };

    // static_assert(mcs::execution::scheduler<task_scheduler>);

}; // namespace mcs::execution::task_v2