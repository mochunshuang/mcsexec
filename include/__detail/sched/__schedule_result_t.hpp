#pragma once

#include <utility>

#include "./__scheduler.hpp"

#include "../factories/__schedule.hpp"

namespace mcs::execution::sched
{
    template <scheduler Sched>
    using schedule_result_t = decltype(factories::schedule(std::declval<Sched>()));

};