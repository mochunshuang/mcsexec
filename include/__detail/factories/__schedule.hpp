#pragma once

#include "../snd/__sender.hpp"

#include "../__core_types.hpp"
#include "../queries/__get_completion_scheduler.hpp"
#include "../queries/__get_env.hpp"

#include <cassert>

namespace mcs::execution::factories
{

    struct schedule_t
    {
        snd::sender decltype(auto) constexpr operator()(auto &&sch) const noexcept
            requires(requires() {
                queries::get_completion_scheduler<set_value_t>(
                    queries::get_env(sch.schedule())) == sch;
            })
        {
            decltype(auto) b = queries::get_completion_scheduler<set_value_t>(
                queries::get_env(sch.schedule()));
            assert(b == sch);

            return sch.schedule();
        }
    };
    inline constexpr schedule_t schedule{}; // NOLINT

}; // namespace mcs::execution::factories
