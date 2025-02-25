#pragma once

#include "./adapt/__continues_on.hpp"
#include "./adapt/__then.hpp"
#include "./adapt/__let_value.hpp"
#include "./adapt/__starts_on.hpp"
#include "./adapt/__on.hpp"
#include "./adapt/__bulk.hpp"
#include "./adapt/__split.hpp"
#include "./adapt/__when_alll.hpp"
#include "./adapt/__into_variant.hpp"
#include "./adapt/__when_all_with_variant.hpp"
#include "./adapt/__stopped_as_optional.hpp"
#include "./adapt/__stopped_as_error.hpp"

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
    using ::mcs::execution::adapt::on_t;
    using ::mcs::execution::adapt::bulk_t;
    using ::mcs::execution::adapt::split_t;
    using ::mcs::execution::adapt::when_all_t;
    using ::mcs::execution::adapt::into_variant_t;
    using ::mcs::execution::adapt::when_all_with_variant_t;
    using ::mcs::execution::adapt::stopped_as_optional_t;
    using ::mcs::execution::adapt::stopped_as_error_t;

    using ::mcs::execution::adapt::continues_on;
    using ::mcs::execution::adapt::then;
    using ::mcs::execution::adapt::upon_error;
    using ::mcs::execution::adapt::upon_stopped;
    using ::mcs::execution::adapt::let_value;
    using ::mcs::execution::adapt::let_error;
    using ::mcs::execution::adapt::let_stopped;
    using ::mcs::execution::adapt::starts_on;
    using ::mcs::execution::adapt::on;
    using ::mcs::execution::adapt::bulk;
    using ::mcs::execution::adapt::bulk_chunked;
    using ::mcs::execution::adapt::bulk_unchunked;
    using ::mcs::execution::adapt::split;
    using ::mcs::execution::adapt::when_all;
    using ::mcs::execution::adapt::into_variant;
    using ::mcs::execution::adapt::when_all_with_variant;
    using ::mcs::execution::adapt::stopped_as_optional;
    using ::mcs::execution::adapt::stopped_as_error;

}; // namespace mcs::execution