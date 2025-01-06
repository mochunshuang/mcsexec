#include "../test_base_head.hpp"

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
struct mem_co_awaiter
{
    int value{};                                    // NOLINT
    auto operator co_await() -> awaiter<bool, bool> // NOLINT
    {
        return {5 + this->value}; // NOLINT
    }
};

template <typename T, bool Noexcept = true>
struct with_as_awaitable
{
    auto as_awaitable(auto &&) noexcept(Noexcept) -> T // NOLINT
    {
        return {};
    }
};

struct co_awaiter
{
    int value{};
};
auto operator co_await(co_awaiter obj) -> awaiter<bool, bool> // NOLINT
{
    return {3 + obj.value};
}

int main()
{
    struct promise_type
    {
    };

    TEST("test") = [] {
        using namespace mcs::execution::awaitables; // NOLINT
        static_assert(not has_as_awaitable<int, promise_type>);
        static_assert(not has_as_awaitable<with_as_awaitable<int>, promise_type>);

        // by { as_awaitable } -> is_awaitable<Promise &>;
        static_assert(
            has_as_awaitable<with_as_awaitable<awaiter<bool, bool>>, promise_type>);

        // by { as_awaitable } -> is_awaitable<Promise &>;
        static_assert(has_as_awaitable<with_as_awaitable<co_awaiter>, promise_type>);
    };

    return 0;
}