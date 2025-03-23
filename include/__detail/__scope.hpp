#pragma once

#include "./scope/__counting_scope.hpp"
#include "./scope/__simple_counting_scope.hpp"
#include "./scope/__nest.hpp"
#include "./scope/__spawn.hpp"
#include "./scope/__spawn_future.hpp"

#include "./scope/__async_scope_token.hpp"

#include "./scope/__let_async_scope.hpp"

namespace mcs::execution
{
    using ::mcs::execution::scope::counting_scope;
    using ::mcs::execution::scope::simple_counting_scope;
    using ::mcs::execution::scope::nest_t;
    using ::mcs::execution::scope::spawn_t;
    using ::mcs::execution::scope::spawn_future_t;

    using ::mcs::execution::scope::nest;
    using ::mcs::execution::scope::spawn;
    using ::mcs::execution::scope::spawn_future;

    using ::mcs::execution::scope::async_scope_token;

    using ::mcs::execution::scope::let_async_scope;

}; // namespace mcs::execution