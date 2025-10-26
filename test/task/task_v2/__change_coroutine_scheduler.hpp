#pragma once

#include "../../test_base_head.hpp"

namespace mcs::execution::task_v2
{
    template <scheduler Sch>
    struct change_coroutine_scheduler
    {
        using type = std::remove_cvref_t<Sch>;
        type scheduler;
    };
    template <scheduler Sch>
    change_coroutine_scheduler(Sch &&) -> change_coroutine_scheduler<Sch>;
}; // namespace mcs::execution::task_v2