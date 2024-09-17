#pragma once
#include "./ctx/__run_loop.hpp"
#include "./ctx/__static_thread_pool.hpp"

namespace mcs::execution
{
    using ::mcs::execution::ctx::run_loop;
    using ::mcs::execution::ctx::static_thread_pool;

}; // namespace mcs::execution