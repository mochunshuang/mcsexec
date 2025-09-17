#include <algorithm>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <iostream>
#include <ostream>
#include <thread>
#include "../test_base_head.hpp"

struct async_awaiter
{
    int value; // NOLINT
    explicit async_awaiter(int v) : value(v) {}

    constexpr static bool await_ready() noexcept // NOLINT
    {
        return false;
    }

    std::noop_coroutine_handle await_suspend(std::coroutine_handle<> h) noexcept // NOLINT
    {
        std::cout << ">>>>> await_suspend......\n";
        std::thread([h] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            h.resume();
        }).detach();
        return std::noop_coroutine(); // NOTE: 挂起 h
    }

    // NOTE: 第二次调度的时候 await_resume 会被调用
    auto await_resume() const noexcept // NOLINT
    {
        std::cout << "await_resume...\n";
        return value + 1;
    }
};

auto &cpu_pool() noexcept // NOLINT
{
    static mcs::execution::static_thread_pool<1> pool;
    return pool;
}

// auto &global_counting_scope() noexcept // NOLINT
// {
//     static ex::counting_scope scope; // NOTE: counting_scope 是不可移动不可复制的
//     return scope;
// }

struct async_operation
{
    ex::counting_scope &scope; // NOLINT
    int value;                 // NOLINT
    async_operation(ex::counting_scope &s, int v) : scope{s}, value(v) {}

    constexpr static bool await_ready() noexcept // NOLINT
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept // NOLINT
    {
        auto in = std::this_thread::get_id();

#if 0
        // NOTE: 死锁的原样是 sndr 写错了
        auto sndr = mcs::execution::schedule(cpu_pool().get_scheduler()) |
                    ex::then([&, in] noexcept {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        if (in != std::this_thread::get_id())
                        {
                            std::cout << ">>>>> work on [other] thdread......\n";
                        }
                        else
                        {
                            std::cout << ">>>>> work on [same] thdread......\n";
                        }

                        h.resume();
                    });

        // NOTE: 死锁了
        // mcs::execution::spawn(
        //     mcs::execution::on(cpu_pool().get_scheduler(), std::move(sndr)),
        //     scope.get_token()); // NOLINT
        // NOTE: 死锁了
        // mcs::execution::spawn(
        //     std::move(sndr) | mcs::execution::continues_on(cpu_pool().get_scheduler()),
        //     scope.get_token()); // NOLINT
        // NOTE: 死锁了
        // mcs::execution::spawn(
        //     mcs::execution::starts_on(cpu_pool().get_scheduler(), std::move(sndr)),
        //     scope.get_token()); // NOLINT

        // NOTE: 结论是什么呢？ . while 与 starts_on 、 on、 starts_on 容易死锁

        std::thread([&] {
            // NOTE: 还是死锁
            //  mcs::execution::spawn(
            //      mcs::execution::on(cpu_pool().get_scheduler(), std::move(sndr)),
            //      scope.get_token()); // NOLINT

            // NOTE: 还是死锁
            // mcs::execution::spawn(std::move(sndr) | mcs::execution::continues_on(
            //                                             cpu_pool().get_scheduler()),
            //                       scope.get_token()); // NOLIN

            // NOTE: 还是死锁
            // mcs::execution::spawn(
            //     mcs::execution::starts_on(cpu_pool().get_scheduler(), std::move(sndr)),
            //     scope.get_token()); // NOLINT

            // NOTE: 还是死锁
            // mcs::execution::spawn(std::move(sndr), scope.get_token());
        }).detach();

        // NOTE: 还是死锁
        //  mcs::execution::spawn(std::move(sndr),
        //                        scope.get_token()); // NOLINT

        // NOTE: 救不了
        // mcs::execution::spawn(std::move(sndr) | mcs::execution::continues_on(
        //                                             ex::__task::inline_scheduler{}),
        //                       scope.get_token()); // NOLINT
#elif 1
        auto sndr = ex::just() | ex::then([=] noexcept {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        if (in != std::this_thread::get_id())
                        {
                            std::cout << ">>>>> work on [other] thdread......\n";
                        }
                        else
                        {
                            std::cout << ">>>>> work on [same] thdread......\n";
                        }

                        std::cout << "work thread: " << std::this_thread::get_id();

                        h.resume();
                    });

#if 0
        std::thread([&] {
            // NOTE: 还是死锁
            // mcs::execution::spawn(
            //     mcs::execution::on(cpu_pool().get_scheduler(), std::move(sndr)),
            //     scope.get_token()); // NOLINT

            // NOTE: 还是死锁
            // mcs::execution::spawn(std::move(sndr) | mcs::execution::continues_on(
            //                                             cpu_pool().get_scheduler()),
            //                       scope.get_token()); // NOLIN

            // NOTE: 还是死锁
            // mcs::execution::spawn(
            //     mcs::execution::starts_on(cpu_pool().get_scheduler(), std::move(sndr)),
            //     scope.get_token()); // NOLINT

            // NOTE: 保证while 循环， starts_on，on, continues_on 不是同一个
            // scheduler 线程才不会死锁 NOTE: 可重入的 锁，或许才是核心啊
            mcs::execution::spawn(std::move(sndr), scope.get_token());
        }).detach();
#endif

        // NOTE: 死锁
        //  mcs::execution::spawn(
        //      mcs::execution::on(cpu_pool().get_scheduler(), std::move(sndr)),
        //      scope.get_token()); // NOLINT

        // NOTE: 死锁
        // mcs::execution::spawn(
        //     mcs::execution::starts_on(cpu_pool().get_scheduler(), std::move(sndr)),
        //     scope.get_token()); // NOLINT

        // NOTE: 死锁
        // mcs::execution::spawn(
        //     std::move(sndr) | mcs::execution::continues_on(cpu_pool().get_scheduler()),
        //     scope.get_token()); // NOLINT

        // NOTE: 不切合线程没事
        mcs::execution::spawn(std::move(sndr), scope.get_token());
#else
        h.resume();
#endif

        // NOTE: 死锁的原因是 mute 不可以重入
        // NOTE: co_await 是哪个线程 被调度，就从哪个线程继续往下走
        std::cout << ">>>>> await_suspend out\n";
    }

    // NOTE: 第二次调度的时候 await_resume 会被调用
    auto await_resume() const noexcept // NOLINT
    {
        return value + 1;
    }
};

int main()
{

    TEST("co_await suspend_never") = [] {
        auto fun = []() noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";
            co_await std::suspend_never{};
            std::cout << "suspend_never affter...\n";
            co_return 0;
        }();
        auto [ret] = mcs::this_thread::sync_wait(std::move(fun)).value();
        EXPECT(ret == 0);
    };

    TEST("co_await async_awaiter") = [] {
        auto fun = []() noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";
            auto ret = co_await async_awaiter{1}; // NOLINT
            std::cout << "suspend_never affter...\n";
            co_return ret;
        }();
        auto [ret] = mcs::this_thread::sync_wait(std::move(fun)).value();
        EXPECT(ret == 2);
    };

    TEST("co_await async_awaiter 3 times") = [] {
        auto fun = []() noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";
            int ret{};
            int times = 3;
            while (times > 0)
            {
                ret += co_await async_awaiter{1}; // NOLINT
                --times;
            }
            std::cout << "suspend_never affter...\n";
            co_return ret;
        }();
        auto [ret] = mcs::this_thread::sync_wait(std::move(fun)).value();
        EXPECT(ret == 6);
    };

    ex::counting_scope scope;
    TEST("co_await async_operation 3 times") = [&] {
        auto fun = [](auto &scope) noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";

            auto in = std::this_thread::get_id();
            std::cout << "[async_operation task] start thread: " << in << '\n';

            int ret{};
            int times = 3;
            while (times > 0)
            {
                ret += co_await async_operation{scope, 1}; // NOLINT
                --times;
            }
            std::cout << "suspend_never affter...\n";

            std::cout << "[async_operation task] end thread: "
                      << std::this_thread::get_id() << '\n';

            EXPECT(in == std::this_thread::get_id());
            co_return ret;
        }(scope);
        auto [ret] = mcs::this_thread::sync_wait(std::move(fun)).value();
        EXPECT(ret == 6);
    };

    std::cout << "\n\n";
    TEST("co_await async_operation 3 times 2 ") = [&] {
        auto fun = [](auto &scope) noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";

            auto in = std::this_thread::get_id();
            std::cout << "[async_operation task] start thread: " << in << '\n';

            int ret{};
            int times = 3;
            while (times > 0)
            {
                ret += co_await async_operation{scope, 1}; // NOLINT
                --times;
            }
            std::cout << "suspend_never affter...\n";

            std::cout << "[async_operation task] end thread: "
                      << std::this_thread::get_id() << '\n';

            EXPECT(in == std::this_thread::get_id());
            co_return ret;
        }(scope);
        // NOTE: 不用说，死锁
        auto [ret] = mcs::this_thread::sync_wait(
                         ex::starts_on(cpu_pool().get_scheduler(), std::move(fun)))
                         .value();
        EXPECT(ret == 6);
    };
    // NOTE: 还是未能解决。 线程切换的问题。 两个线程池的测试是OK的。

    // NOTE: 但是就是无法绑定 到 当个线程池中吗？
    TEST("co_await + while") = [&] {
        mcs::execution::static_thread_pool<1> other_pool;
        auto main_in = std::this_thread::get_id();
        auto thread_in = cpu_pool()[0].thread_id();

        auto fun = [](auto &main_in, auto &other_pool,
                      auto &thread_in) noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";

            auto in = std::this_thread::get_id();
            EXPECT(in == thread_in);

            int ret{};
            int times = 3;
            while (times > 0)
            {
                // NOTE: 用先线程 push task 避免了 死锁
                ret += co_await (other_pool.get_scheduler().schedule() |
                                 ex::continues_on(cpu_pool().get_scheduler()) |
                                 ex::then([]() { return 2; })); // NOLINT
                std::cout << " ret + : times: " << times << "\n";
                --times;
            }
            std::cout << "suspend_never affter...\n";
            EXPECT(thread_in == std::this_thread::get_id());
            co_return ret;
        }(main_in, other_pool, thread_in);

        auto [ret] = mcs::this_thread::sync_wait(
                         ex::starts_on(cpu_pool().get_scheduler(), std::move(fun)))
                         .value();
        EXPECT(ret == 6);
    };

    // NOTE: 必须如此
    mcs::this_thread::sync_wait(scope.join());
    std::cout << "main done\n";
    return 0;
}