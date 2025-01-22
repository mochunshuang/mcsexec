#pragma once

#include "./__any_scheduler.hpp"
#include "../sched/__scheduler.hpp"

namespace mcs::execution::task
{
    template <typename>
    struct scheduler_of
    {
        using type = any_scheduler;
    };
    template <typename Context>
        requires requires { typename Context::scheduler_type; }
    struct scheduler_of<Context>
    {
        using type = typename Context::scheduler_type;
        static_assert(sched::scheduler<type>,
                      "The type alias scheduler_type needs to refer to a scheduler");
    };

    template <typename Context>
    using scheduler_of_t = typename scheduler_of<Context>::type;

}; // namespace mcs::execution::task