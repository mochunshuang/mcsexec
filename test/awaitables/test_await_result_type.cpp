#include "../test_base_head.hpp"

template <typename T>
struct typed_awaiter
{
    auto await_ready() -> bool // NOLINT
    {
        return {};
    }
    auto await_suspend() -> bool // NOLINT
    {
        return {};
    }
    auto await_resume() -> T // NOLINT
    {
        return {};
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
struct mem_co_awaiter
{
    int value{};                                    // NOLINT
    auto operator co_await() -> awaiter<bool, bool> // NOLINT
    {
        return {5 + this->value}; // NOLINT
    }
};

struct promise_type
{
    auto await_transform(typed_awaiter<bool>) // NOLINT
    {
        return mem_co_awaiter{5}; // NOLINT
    }
};

int main()
{
    TEST("base") = [] {
        using namespace mcs::execution::awaitables; // NOLINT

        // NOTE: typed_awaiter<T>.await_resume() => T
        static_assert(
            std::same_as<int, await_result_type<typed_awaiter<int>, promise_type>>);
        static_assert(
            std::same_as<double, await_result_type<typed_awaiter<double>, promise_type>>);

        // as await_transform match
        // => mem_co_awaiter => awaiter<bool, bool>.await_resume() => void
        static_assert(
            std::same_as<void, await_result_type<typed_awaiter<bool>, promise_type>>);
    };

    return 0;
}