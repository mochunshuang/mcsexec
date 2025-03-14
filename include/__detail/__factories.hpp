#pragma once

#include "./factories/__schedule.hpp"
#include "./factories/__just.hpp"
#include "./factories/__read_env.hpp"
#include "./factories/__write_env.hpp"

namespace mcs::execution
{
    using ::mcs::execution::factories::schedule_t;
    using ::mcs::execution::factories::just_t;
    using ::mcs::execution::factories::just_error_t;
    using ::mcs::execution::factories::just_stopped_t;
    using ::mcs::execution::factories::read_env_t;
    using ::mcs::execution::factories::write_env_t;

    using ::mcs::execution::factories::schedule;
    using ::mcs::execution::factories::just;
    using ::mcs::execution::factories::just_error;
    using ::mcs::execution::factories::just_stopped;
    using ::mcs::execution::factories::read_env;
    using ::mcs::execution::factories::write_env;

}; // namespace mcs::execution