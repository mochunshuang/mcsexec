#include "../test_base_head.hpp"

using namespace mcs::execution::awaitables; // NOLINT

struct promise_with_await_transform : with_await_transform<promise_with_await_transform>
{
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

template <typename T, bool Noexcept = true>
struct with_as_awaitable
{
    auto as_awaitable(auto &&) noexcept(Noexcept) -> T // NOLINT
    {
        return {};
    }
};

int main()
{

    TEST("test_with_await_transform") = [] {
        promise_with_await_transform promise{};

        static_assert(noexcept(promise.await_transform(awaiter<bool, void>{})));
        auto a1{promise.await_transform(awaiter<bool, void>{})};
        static_assert(std::same_as<awaiter<bool, void>, decltype(a1)>);

        static_assert(noexcept(
            promise.await_transform(with_as_awaitable<awaiter<bool, void>, true>{})));
        static_assert(not noexcept(
            promise.await_transform(with_as_awaitable<awaiter<bool, void>, false>{})));

        auto a2{promise.await_transform(with_as_awaitable<awaiter<bool, void>>{})};
        static_assert(std::same_as<awaiter<bool, void>, decltype(a2)>);
    };

    return 0;
}