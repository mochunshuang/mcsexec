#pragma once

#include "../recv/__receiver.hpp"
#include "../recv/__set_value.hpp"
#include "../snd/__sender.hpp"
#include "../queries/__get_completion_scheduler.hpp"
#include "../cmplsigs/__completion_signatures.hpp"
#include "../sched/__scheduler.hpp"

namespace mcs::execution::__task
{
    /*!
     * @brief Scheduler completing immmediately when started on the same thread
     *
     * The class `inline_scheduler` is used to prevent any actual schedulering.
     * It does have a scheduler interface but it completes synchronously on
     * the thread on which it gets `start`ed before returning from `start`.
     * The implication is that any blocking working gets executed on the
     * calling thread. Also, if there is lot of synchronous work repeatedly
     * getting scheduled using `inline_scheduler` it is possible to get a
     * stack overflow.
     *
     * In general, any use of `inline_scheduler` should receive a lot of
     * attention as it is fairly easy to create subtle bugs using this scheduler.
     */
    struct inline_scheduler
    {
        struct env
        {
            [[nodiscard]] static constexpr inline_scheduler query(
                const queries::get_completion_scheduler_t<set_value_t>
                    & /*unused*/) noexcept
            {
                return {};
            }
        };
        template <recv::receiver Receiver>
        struct state
        {
            using operation_state_concept = operation_state_t;
            std::remove_cvref_t<Receiver> receiver; // NOLINT
            void start() & noexcept
            {
                recv::set_value(std::move(receiver));
            }
        };
        struct sender
        {
            using sender_concept = sender_t;
            using completion_signatures = cmplsigs::completion_signatures<set_value_t()>;

            [[nodiscard]] env get_env() const noexcept // NOLINT
            {
                return {};
            }
            template <recv::receiver Receiver>
            state<Receiver> connect(Receiver &&receiver) noexcept
            {
                return {std::forward<Receiver>(receiver)};
            }
        };
        static_assert(snd::sender<sender>);

        using scheduler_concept = scheduler_t;
        inline_scheduler() = default;

        template <typename Scheduler>
        explicit inline_scheduler(Scheduler && /*unused*/) // NOLINT
        {
            static_assert(sched::scheduler<Scheduler>);
        }
        static constexpr sender schedule() noexcept
        {
            return {};
        }
        bool operator==(const inline_scheduler &) const = default;
    };
    static_assert(sched::scheduler<inline_scheduler>);
}; // namespace mcs::execution::__task