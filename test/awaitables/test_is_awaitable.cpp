
#include "../test_base_head.hpp"
#include <coroutine>

using namespace mcs::execution::awaitables; // NOLINT
struct has_await_transform
{
    template <typename Expr>
    auto await_transform(Expr &&expr) // NOLINT
    {
        return std::forward<Expr>(expr);
    }
};

struct ValidAwaitable
{
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

    ValidAwaiter operator co_await()
    {
        return ValidAwaiter{};
    }
};

struct InvalidAwaitable
{
};

struct GlobalAwaiter
{
    bool await_ready() noexcept // NOLINT
    {
        return true;
    }
    void await_suspend(std::coroutine_handle<>) noexcept {} // NOLINT
    int await_resume() noexcept                             // NOLINT
    {
        return 1;
    }
};

struct GlobalAwaitable
{
    friend GlobalAwaiter operator co_await(GlobalAwaitable /*unused*/)
    {
        return GlobalAwaiter{};
    }
};

struct DirectAwaiter
{
    bool await_ready() noexcept // NOLINT
    {
        return true;
    }
    void await_suspend(std::coroutine_handle<>) noexcept {} // NOLINT
    int await_resume() noexcept                             // NOLINT
    {
        return 1;
    }
};

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

struct co_awaiter
{
    int value{};
};
auto operator co_await(co_awaiter obj) -> awaiter<bool, bool> // NOLINT
{
    return {3 + obj.value};
}

struct mem_co_awaiter
{
    int value{};                                    // NOLINT
    auto operator co_await() -> awaiter<bool, bool> // NOLINT
    {
        return {5 + this->value}; // NOLINT
    }
};

int main()
{
    TEST("is_awaitable") = [] {
        static_assert(is_awaitable<ValidAwaitable, has_await_transform>);

        static_assert(not is_awaitable<InvalidAwaitable, has_await_transform>);

        {
            static_assert(is_awaitable<GlobalAwaitable, has_await_transform>);
            // Note: GlobalAwaiter form GlobalAwaitable.co_await
            static_assert(is_awaiter<GlobalAwaiter, has_await_transform>);
        }
        static_assert(not is_awaiter<has_await_transform, has_await_transform>);

        static_assert(is_awaitable<GlobalAwaitable, GlobalAwaiter>);

        static_assert(is_awaitable<DirectAwaiter, DirectAwaiter>);
        static_assert(is_awaitable<GlobalAwaitable, DirectAwaiter>);
    };

    TEST("test_is_awaitable") = [] {
        struct promise_type;
        static_assert(is_awaitable<awaiter<bool, void>, promise_type>);
        static_assert(is_awaitable<awaiter<bool, bool>, promise_type>);
        static_assert(is_awaitable<co_awaiter, promise_type>);
        static_assert(is_awaitable<mem_co_awaiter, promise_type>);
        static_assert(not is_awaitable<awaiter<bool, double>, promise_type>);
    };

    TEST("await_suspend(std::coroutine_handle<>) is good") = [] {
        auto await_suspend = [](std::coroutine_handle<>) {
        };
        await_suspend(std::coroutine_handle<>{});
        await_suspend(std::coroutine_handle<int>{});
        await_suspend(std::coroutine_handle<double>{});

        struct unspecified_class;
        await_suspend(std::coroutine_handle<unspecified_class>{});
    };

    return 0;
}