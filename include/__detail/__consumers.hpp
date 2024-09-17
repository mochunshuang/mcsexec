#pragma once
#include "./consumers/__sync_wait.hpp"

namespace mcs::this_thread
{
    using mcs::execution::consumers::__sync_wait::sync_wait_t;
    using mcs::execution::consumers::sync_wait; // NOLINT

}; // namespace mcs::this_thread