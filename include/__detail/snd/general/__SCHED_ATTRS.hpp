#pragma once

#include "../../__core_types.hpp"

#include "../../queries/__get_domain.hpp"
#include "../../queries/__get_completion_scheduler.hpp"

namespace mcs::execution::snd::general
{
    template <typename Scheduler>
    class SCHED_ATTRS // NOLINT
    {

        Scheduler sched; // NOLINT

      public:
        template <typename S>
        explicit SCHED_ATTRS(S &&s) : sched(std::forward<S>(s))
        {
        }
        // delete by SCHED_ATTRS(S &&s)
        SCHED_ATTRS(const SCHED_ATTRS &) = delete;
        SCHED_ATTRS(SCHED_ATTRS &&) noexcept = delete;

        ~SCHED_ATTRS() noexcept = default;
        SCHED_ATTRS &operator=(const SCHED_ATTRS &) = default;
        SCHED_ATTRS &operator=(SCHED_ATTRS &&) noexcept = default;

        template <typename Tag>
            requires(std::is_same_v<Tag, set_value_t> or
                     std::is_same_v<Tag, set_stopped_t>)
        constexpr auto query(
            queries::get_completion_scheduler_t<Tag> const & /*q*/) const noexcept
        {
            return sched;
        }

        constexpr auto query(queries::get_domain_t const &q) const noexcept
        {
            return sched.query(q);
        }
    };

    template <typename T>
    SCHED_ATTRS(T &&) -> SCHED_ATTRS<std::remove_cvref_t<T>>;

}; // namespace mcs::execution::snd::general