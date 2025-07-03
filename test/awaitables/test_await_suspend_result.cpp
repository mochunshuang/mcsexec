#include "../test_base_head.hpp"

using namespace mcs::execution::awaitables; // NOLINT

struct promise_type
{
};

int main()
{
    TEST("base") = [] {
        // NOTE: only void、bool、std::coroutine_handle<auto> can be await_suspend_result
        static_assert(await_suspend_result<void>);
        static_assert(await_suspend_result<bool>);
        static_assert(await_suspend_result<std::coroutine_handle<>>);
        static_assert(await_suspend_result<std::coroutine_handle<promise_type>>);
        static_assert(not await_suspend_result<int>);
    };

    return 0;
}