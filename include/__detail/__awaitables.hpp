#pragma once

#include "./awaitables/__await_suspend_result.hpp"
#include "./awaitables/__is_awaiter.hpp"
#include "./awaitables/__is_awaitable.hpp"
#include "./awaitables/__with_await_transform.hpp"
#include "./awaitables/__env_promise.hpp"
#include "./awaitables/__await_result_type.hpp"
#include "./awaitables/__as_awaitable.hpp"
#include "./awaitables/__with_awaitable_senders.hpp"

namespace mcs::execution
{
    using awaitables::await_suspend_result;
    using awaitables::is_awaiter;
    using awaitables::is_awaitable;

    using awaitables::with_await_transform;
    using awaitables::env_promise;

    // await-result-type
    using awaitables::await_result_type;

    using awaitables::as_awaitable;
    using awaitables::with_awaitable_senders;
}; // namespace mcs::execution
