#pragma once

#include "./__await_suspend_result.hpp"

namespace mcs::execution::awaitables
{

    template <class A, class Promise>
    concept is_awaiter = // exposition only
        requires(A &a, std::coroutine_handle<Promise> h) {
            a.await_ready() ? 1 : 0;
            { a.await_suspend(h) } -> await_suspend_result;
            a.await_resume();
        };

}; // namespace mcs::execution::awaitables
