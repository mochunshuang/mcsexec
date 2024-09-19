#pragma once

#include "./__query_or_default.hpp"
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
        template <typename S> // need by get_env() -> const &
        explicit SCHED_ATTRS(S &&s) : sched(std::forward<S>(s))
        {
        }
        // delete by SCHED_ATTRS(S &&s). so Forced generation
        SCHED_ATTRS(const SCHED_ATTRS &) = default;
        SCHED_ATTRS(SCHED_ATTRS &&) noexcept = default;

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
            // Note: :ctx::run_loop::scheduler' has no member named 'query'
            // TODO(mcs): just()也没有，或许是
            // return sched.query(q);
            return query_or_default(q, sched, default_domain());
        }
    };

    template <typename T>
    SCHED_ATTRS(T &&) -> SCHED_ATTRS<std::remove_cvref_t<T>>;

}; // namespace mcs::execution::snd::general