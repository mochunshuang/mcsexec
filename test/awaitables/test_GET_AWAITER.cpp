
#include "../test_base_head.hpp"
#include <coroutine>

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

int main()
{

    // Note: summary:
    // Note: 1. promise_type'await_transform can invoke, result is awaiter of fun return
    // Note: 2. promise_type can't invoke, result is old awaiter itself
    // Note: 3. result awaiter
    //              => operator co_await() => new awaiter
    //              => or old awaiter itself
    TEST("test_GET_AWAITER") = [] {
        struct promise_type
        {
            auto await_transform(awaiter<bool, bool> a) // NOLINT
            {
                return awaiter<bool, std::coroutine_handle<>>{2 + a.value};
            }

            auto await_transform(awaiter<bool, int> a) // NOLINT
            {
                return co_awaiter{4 + a.value};
            }
            auto await_transform(awaiter<bool, long> a) // NOLINT
            {
                return mem_co_awaiter{5 + a.value}; // NOLINT
            }
            auto await_transform(typed_awaiter<bool>) // NOLINT
            {
                return mem_co_awaiter{5}; // NOLINT
            }
            // Note: can't await_transform for type awaiter<bool, void>
        } promise{};

        using namespace mcs::execution::awaitables::__detail;  // NOLINT
        auto a1{GET_AWAITER(awaiter<bool, void>{1}, promise)}; // NOLINT
        static_assert(std::same_as<awaiter<bool, void>, decltype(a1)>);
        EXPECT(a1.value == 1);

        auto a2{GET_AWAITER(awaiter<bool, bool>{1}, promise)}; // NOLINT
        static_assert(std::same_as<awaiter<bool, std::coroutine_handle<>>, decltype(a2)>);
        EXPECT(a2.value == 3);

        auto a3{GET_AWAITER(co_awaiter{1}, promise)}; // NOLINT
        static_assert(std::same_as<awaiter<bool, bool>, decltype(a3)>);
        EXPECT(a3.value == 4);

        // awaiter: 1
        // -> promise.await_transform(awaiter<bool, int> a): 1+4
        // -> auto operator co_await(co_awaiter obj) -> awaiter<bool, bool> : 1+4+3
        auto a4{GET_AWAITER(awaiter<bool, int>{1}, promise)}; // NOLINT
        static_assert(std::same_as<awaiter<bool, bool>, decltype(a4)>);
        EXPECT(a4.value == (1 + 4 + 3));

        // 1 + 5
        // -> auto operator co_await(co_awaiter obj) -> awaiter<bool, bool> : 1 + 5 + 5
        auto a5{GET_AWAITER(awaiter<bool, long>{1}, promise)}; // NOLINT
        static_assert(std::same_as<awaiter<bool, bool>, decltype(a5)>);
        EXPECT(a5.value == (1 + 5 + 5));

        // 1
        // -> auto operator co_await(co_awaiter obj) -> awaiter<bool, bool> : 1 + 5
        auto a6{GET_AWAITER(mem_co_awaiter{1}, promise)}; // NOLINT
        static_assert(std::same_as<awaiter<bool, bool>, decltype(a6)>);
        EXPECT(a6.value == 6);

        // 5
        // -> auto operator co_await(co_awaiter obj) -> awaiter<bool, bool> : 5 + 5
        auto a7{GET_AWAITER(typed_awaiter<bool>{}, promise)};
        static_assert(std::same_as<awaiter<bool, bool>, decltype(a7)>);
        EXPECT(a7.value == (5 + 5));
    };

    return 0;
}