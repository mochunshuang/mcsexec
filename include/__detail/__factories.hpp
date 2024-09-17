#pragma once

#include "./factories/__schedule.hpp"
#include "./factories/__just.hpp"

namespace mcs::execution
{
    using ::mcs::execution::factories::schedule_t;
    using ::mcs::execution::factories::just_t;
    using ::mcs::execution::factories::just_error_t;
    using ::mcs::execution::factories::just_stopped_t;

    using ::mcs::execution::factories::schedule;
    using ::mcs::execution::factories::just;
    using ::mcs::execution::factories::just_error;
    using ::mcs::execution::factories::just_stopped;

}; // namespace mcs::execution