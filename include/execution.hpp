#ifndef __MCS__EXECUTION__H__
#define __MCS__EXECUTION__H__

#include "./__detail/__awaitables.hpp"

#include "./__detail/__queries.hpp"
#include "./__detail/__snd.hpp"
#include "./__detail/__recv.hpp"
#include "./__detail/__sched.hpp"

#include "./__detail/__conn.hpp"

#include "./__detail/__pipeable.hpp"

#include "./__detail/__factories.hpp"

#include "./__detail/__adapt.hpp"

#include "./__detail/__ctx.hpp"

#include "./__detail/__consumers.hpp"

#include "./__detail/__utils.hpp"

#include "./__detail/__tool.hpp"

#include "./__detail/__task.hpp"
#include "__detail/ctx/__run_loop.hpp"
#include "__detail/queries/__forwarding_query.hpp"

#include "./__detail/__scope.hpp"

namespace mcs::execution
{
    // ==============Concepts===================
    using sched::scheduler;

    using snd::sender;
    using snd::sender_in;
    using snd::sender_to;

    using recv::receiver;
    using recv::receiver_of;

    using opstate::operation_state;

    // ==============Utility components===================
    // Execution contexts
    using ctx::run_loop;

    // Execution domains
    using snd::default_domain;
    using snd::transform_sender;
    using snd::transform_env;
    using snd::apply_sender;

    // Forward progress guarantee
    using ::mcs::execution::forward_progress_guarantee;

    // Environments
    using queries::prop;
    using queries::env;
    using queries::get_env;

    // Queries
    using queries::forwarding_query;
    using queries::get_allocator;
    using queries::get_stop_token;
    using queries::get_domain;
    using queries::get_scheduler;
    using queries::get_delegation_scheduler;
    using queries::get_completion_scheduler;
    using queries::get_forward_progress_guarantee;

    // Completion signatures
    using execution::completion_signatures;
    using snd::get_completion_signatures;
    using tfxcmplsigs::transform_completion_signatures;
    using tfxcmplsigs::transform_completion_signatures_of;
    using snd::tag_of_t;
    using cmplsigs::value_types_of_t;
    using cmplsigs::error_types_of_t;
    using cmplsigs::sends_stopped;

    // Coroutine utility
    using awaitables::as_awaitable;
    using awaitables::with_awaitable_senders;

    // ==============Core operations===================
    // Operation state
    using conn::connect;
    using opstate::start;

    // Completion functions
    using recv::set_value;
    using recv::set_error;
    using recv::set_stopped;

    // Sender algorithms
    using factories::just;
    using factories::just_error;
    using factories::just_stopped;
    using factories::read_env;
    using factories::schedule;

    // Pipeable sender adaptors
    using pipeable::sender_adaptor_closure;

    // Sender adaptors
    using adapt::starts_on;
    using adapt::continues_on;
    using adapt::on;
    using adapt::schedule_from;
    using adapt::then;
    using adapt::upon_error;
    using adapt::upon_stopped;
    using adapt::let_value;
    using adapt::let_error;
    using adapt::let_stopped;
    using adapt::bulk;
    using adapt::bulk_chunked;
    using adapt::bulk_unchunked;
    using adapt::split;
    using adapt::when_all;
    using adapt::when_all_with_variant;
    using adapt::into_variant;
    using adapt::stopped_as_optional;
    using adapt::stopped_as_error;

}; // namespace mcs::execution

namespace mcs::this_thread
{
    // Sender consumers
    using this_thread::sync_wait;
    using this_thread::sync_wait_with_variant;

}; // namespace mcs::this_thread

#endif // __MCS__EXECUTION__H__d