#pragma once

#include "./adapt/__continues_on.hpp"
#include "./adapt/__then.hpp"
#include "./adapt/__let_value.hpp"
#include "./adapt/__starts_on.hpp"

namespace mcs::execution
{
    using ::mcs::execution::adapt::continues_on_t;
    using ::mcs::execution::adapt::then_t;
    using ::mcs::execution::adapt::upon_error_t;
    using ::mcs::execution::adapt::upon_stopped_t;
    using ::mcs::execution::adapt::let_value_t;
    using ::mcs::execution::adapt::let_error_t;
    using ::mcs::execution::adapt::let_stopped_t;
    using ::mcs::execution::adapt::starts_on_t;

    using ::mcs::execution::adapt::continues_on;
    using ::mcs::execution::adapt::then;
    using ::mcs::execution::adapt::upon_error;
    using ::mcs::execution::adapt::upon_stopped;
    using ::mcs::execution::adapt::let_value;
    using ::mcs::execution::adapt::let_error;
    using ::mcs::execution::adapt::let_stopped;
    using ::mcs::execution::adapt::starts_on;

}; // namespace mcs::execution