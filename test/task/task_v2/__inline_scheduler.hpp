#pragma once

#include "../../test_base_head.hpp"
#include <algorithm>
#include <type_traits>

namespace mcs::execution
{
    namespace task_v2
    {
        // inline_scheduler is a class that models scheduler [exec.scheduler]. All objects
        // of type inline_scheduler are equal.
        //  [exec.inline.scheduler]
        struct inline_scheduler // NOLINT
        {
            template <recv::receiver R>
            class inline_state // exposition only // NOLINT
            {
                R rcvr_;

              public:
                constexpr explicit inline_state(R &&r) noexcept : rcvr_(std::move(r)) {}
                using operation_state_concept = operation_state_t;

                // the expression start(o) is equivalent to set_value(std::move(REC(o))).
                void start() & noexcept
                {
                    recv::set_value(std::move(rcvr_));
                }
            };

            /*
            inline-sender is an exposition-only type that satisfies sender. The type
            completion_signatures_of_t<inline-sender> is
            completion_signatures<set_value_t()>.
            */
            struct inline_sender // exposition only // NOLINT
            {
                using sender_concept = sender_t;
                using completion_signatures = completion_signatures<set_value_t()>;

                // NOTE: 指定 inline_state 做实际操作
                template <receiver Rcvr>
                constexpr auto connect(Rcvr rcvr) noexcept(noexcept(auto(rcvr)))
                    -> inline_state<std::decay_t<Rcvr>>
                {
                    return {std::move(rcvr)};
                }

                struct env
                {

                    [[nodiscard]] constexpr auto query( // NOLINT
                        queries::get_completion_scheduler_t<set_value_t> /*unused*/)
                        const noexcept
                    {
                        // the expression
                        // get_completion_scheduler<set_value_t>(get_env(sndr)) has type
                        // inline_scheduler
                        return inline_scheduler{};
                    }
                };

                // NOLINTNEXTLINE
                [[nodiscard]] constexpr auto get_env() const noexcept -> env
                {
                    return {};
                }
            };
            // is_sender
            static_assert(mcs::execution::snd::is_sender<inline_sender>);
            static_assert(mcs::execution::sender<inline_sender>);

          public:
            using scheduler_concept = scheduler_t;
            inline_scheduler() = default;

            static constexpr inline_sender schedule() noexcept
            {
                return {};
            }
            constexpr bool operator==(const inline_scheduler &) const noexcept = default;
        };

        static_assert(mcs::execution::scheduler<inline_scheduler>);
        static_assert(mcs::execution::sender<inline_scheduler::inline_sender>);

    }; // namespace task_v2

}; // namespace mcs::execution