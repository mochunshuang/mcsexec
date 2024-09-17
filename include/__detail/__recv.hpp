#pragma once

#include "./recv/__receiver.hpp"
#include "./recv/__receiver_of.hpp"
#include "./recv/__set_error.hpp"
#include "./recv/__set_stopped.hpp"
#include "./recv/__set_value.hpp"

namespace mcs::execution
{
    using ::mcs::execution::recv::receiver;
    using ::mcs::execution::recv::receiver_of;

    using ::mcs::execution::recv::set_error_t;
    using ::mcs::execution::recv::set_stopped_t;
    using ::mcs::execution::recv::set_value_t;

    using ::mcs::execution::recv::set_error;
    using ::mcs::execution::recv::set_stopped;
    using ::mcs::execution::recv::set_value;

}; // namespace mcs::execution