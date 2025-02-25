#pragma once

#include "./queries/__forwarding_query.hpp"
#include "./queries/__get_allocator.hpp"
#include "./queries/__get_completion_scheduler.hpp"
#include "./queries/__get_delegation_scheduler.hpp"
#include "./queries/__get_forward_progress_guarantee.hpp"
#include "./queries/__get_domain.hpp"
#include "./queries/__get_env.hpp"
#include "./queries/__get_scheduler.hpp"
#include "./queries/__get_stop_token.hpp"
#include "./queries/__env_of_t.hpp"

#include "./queries/__forwar_dingquery_concept.hpp"

#include "./queries/__prop.hpp"
#include "./queries/__env.hpp"

namespace mcs::execution
{
    using ::mcs::execution::queries::forwarding_query_t;
    using ::mcs::execution::queries::get_allocator_t;
    using ::mcs::execution::queries::get_stop_token_t;
    using ::mcs::execution::queries::get_env_t;
    using ::mcs::execution::queries::get_domain_t;
    using ::mcs::execution::queries::get_scheduler_t;
    using ::mcs::execution::queries::get_delegation_scheduler_t;
    using ::mcs::execution::queries::get_completion_scheduler_t;
    using ::mcs::execution::queries::get_forward_progress_guarantee_t;

    using ::mcs::execution::queries::prop;
    using ::mcs::execution::queries::env;

    using ::mcs::execution::queries::forwarding_query;
    using ::mcs::execution::queries::get_allocator;
    using ::mcs::execution::queries::get_stop_token;
    using ::mcs::execution::queries::get_env;
    using ::mcs::execution::queries::get_domain;
    using ::mcs::execution::queries::get_scheduler;
    using ::mcs::execution::queries::get_delegation_scheduler;
    using ::mcs::execution::queries::get_completion_scheduler;
    using ::mcs::execution::queries::get_forward_progress_guarantee;

    using ::mcs::execution::queries::completion_tag;
    using ::mcs::execution::queries::env_of_t;

    using ::mcs::execution::queries::forwardingquery;

}; // namespace mcs::execution