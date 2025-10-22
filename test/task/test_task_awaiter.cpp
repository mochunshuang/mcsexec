#include "../test_base_head.hpp"
#include <cassert>
#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <thread>
#include <utility>

// NOLINTBEGIN
struct this_coroutine
{
    static constexpr bool await_ready() noexcept
    {
        return false;
    }
    constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept
    {
        h_ = h;
        return h;
    }
    constexpr auto await_resume() noexcept
    {
        return h_;
    }

  private:
    std::coroutine_handle<> h_;
};

struct suspend_this_coroutine
{
    static constexpr bool await_ready() noexcept
    {
        return false;
    }
    constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
    {
        return std::noop_coroutine();
    }
    constexpr auto await_resume() noexcept {}
};

static constexpr auto &cpu_pool() noexcept
{
    namespace ex = mcs::execution;
    static ex::static_thread_pool<3> pool;
    return pool;
}
static constexpr auto &counting_scope() noexcept
{
    namespace ex = mcs::execution;
    static ex::counting_scope scope;
    return scope;
}
// NOLINTEND

int main()
try
{
    TEST("BASE") = [] {
        auto task = []() -> ex::task<int> {
            [[maybe_unused]] auto h = co_await this_coroutine{}; // NOLINT
            co_return 1;
        }() | ex::upon_error([](auto) noexcept {
                                UNEXPECT("unreachable");
                                return -1;
                            });
        auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
        EXPECT(ret == 1);
    };

    TEST("get_handle_awaiter + std::suspend_always") = [] {
        auto task = []() -> ex::task<int> {
            [[maybe_unused]] auto h = co_await this_coroutine{}; // NOLINT
            int count = 3;
            auto call_back =
                ex::just() | ex::then([&]() noexcept {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    --count;
                    h.resume();
                });
            while (count != 0)
            {
                std::jthread([=] {
                    ex::spawn(ex::starts_on(cpu_pool().get_scheduler(), call_back),
                              counting_scope().get_token());
                }).detach();
                // co_await suspend_this_coroutine{};
                co_await std::suspend_always{};
            }

            co_return count;
        }() | ex::upon_error([](auto) noexcept {
                                UNEXPECT("unreachable");
                                return -1;
                            });
        auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
        EXPECT(ret == 0);
    };

    mcs::this_thread::sync_wait(counting_scope().join());
    return 0;
}
catch (const std::exception &e)
{
    std::cout << "catch exception: " << e.what() << '\n';
    return -1;
}
