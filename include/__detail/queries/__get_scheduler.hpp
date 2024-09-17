#pragma once

#include "./__forwarding_query.hpp"

#include "../sched/__scheduler.hpp"

namespace mcs::execution::queries
{

    struct get_scheduler_t : forwarding_query_t
    {
        using __self_t = get_scheduler_t;
        template <typename T>
            requires requires(T &&env) {
                { std::as_const(env).query(std::declval<__self_t>()) } noexcept;
            }
        constexpr auto operator()(T &&env) const noexcept -> sched::scheduler auto
        {
            return std::as_const(env).query(*this);
        }
    };
    inline constexpr get_scheduler_t get_scheduler{}; // NOLINT

}; // namespace mcs::execution::queries