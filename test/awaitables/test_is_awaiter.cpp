
#include "../test_base_head.hpp"
#include <coroutine>

struct ValidAwaiter
{
    bool await_ready() // NOLINT
    {
        return true;
    }
    void await_suspend(std::coroutine_handle<>) {} // NOLINT
    int await_resume()                             // NOLINT
    {
        return 1;
    }
};

struct InvalidAwaiter
{
    void await_suspend(std::coroutine_handle<>) {} // NOLINT
    int await_resume()                             // NOLINT
    {
        return 1;
    }
    // 缺少 await_ready()
};

using namespace mcs::execution::awaitables; // NOLINT

template <typename Ready, typename Suspend, typename... Resume>
struct awaiter
{
    int value{};                // NOLINT
    auto await_ready() -> Ready // NOLINT
    {
        return {};
    }
    auto await_suspend(auto) -> Suspend // NOLINT
    {
        return {};
    }
    auto await_resume(Resume...) -> void {} // NOLINT
};

template <typename Promise>
struct awaiter_with_handle
{
    auto await_ready() -> bool // NOLINT
    {
        return {};
    }
    auto await_suspend(std::coroutine_handle<Promise>) -> bool // NOLINT
    {
        return {};
    }
    auto await_resume() -> void {} // NOLINT
};

int main()
{
    using namespace mcs::execution::awaitables; // NOLINT
    TEST("is_awaiter") = [] {
        static_assert(is_awaiter<ValidAwaiter, void>);

        static_assert(not is_awaiter<InvalidAwaiter, void>);
    };

    TEST("is_awaiter with custom Promise") = [] {
        struct MyPromise
        {
            using promise_type = MyPromise; // NOLINT
            std::coroutine_handle<> handle;
        };
        static_assert(is_awaiter<ValidAwaiter, MyPromise>);
    };

    TEST("test_is_awaiter") = [] {
        struct promise_type;
        struct type;

        static_assert(not is_awaiter<int, promise_type>);

        static_assert(is_awaiter<awaiter<bool, void>, promise_type>);
        static_assert(is_awaiter<awaiter<bool, bool>, promise_type>);
        static_assert(is_awaiter<awaiter<bool, std::coroutine_handle<>>, promise_type>);
        static_assert(
            is_awaiter<awaiter<bool, std::coroutine_handle<promise_type>>, promise_type>);

        static_assert(not is_awaiter<awaiter<type, void>, promise_type>);
        static_assert(not is_awaiter<awaiter<bool, type>, promise_type>);
        static_assert(not is_awaiter<awaiter<bool, void, int>, promise_type>);

        static_assert(is_awaiter<awaiter_with_handle<promise_type>, promise_type>);
        static_assert(is_awaiter<awaiter_with_handle<void>, promise_type>);
        static_assert(not is_awaiter<awaiter_with_handle<type>, promise_type>);
    };
    return 0;
}