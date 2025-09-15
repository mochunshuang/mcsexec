#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "../test_base_head.hpp"

auto &globle_counting_scope() noexcept // NOLINT
{
    static ex::counting_scope scope;
    return scope;
}

auto &cpu_pool() noexcept // NOLINT
{
    static mcs::execution::static_thread_pool<1> pool;
    return pool;
}

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

                        std::cout << "work thread: " << std::this_thread::get_id()
                                  << '\n';

                        h.resume();
                    });
        // mcs::execution::spawn(
        //     std::move(sndr) | mcs::execution::continues_on(cpu_pool().get_scheduler()),
        //     scope.get_token());

        mcs::execution::spawn(std::move(sndr), scope.get_token());

        std::cout << ">>>>> await_suspend out\n";
    }

    auto await_resume() const noexcept // NOLINT
    {
        return value + 1;
    }
};

int main()
{
    auto &pool = cpu_pool();
    ex::counting_scope &scope = globle_counting_scope();

    TEST("thread + continues_on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        auto task = ex::then([&]() noexcept {
            assert(std::this_thread::get_id() == pool_id);
            calld = true;
            // NOTE: 注意是 pool 的线程来到这这里
        });
        ex::spawn(ex::continues_on(ex::just(), pool.get_scheduler()) | std::move(task),
                  scope.get_token());
        while (not calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
    };

    // NOTE: 死锁测试
    TEST("deadlock + continues_on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        bool inner_calld = false;
        auto task = ex::then([&]() noexcept {
            assert(std::this_thread::get_id() == pool_id);
            calld = true;

            constexpr auto test_deadlock = true; // NOLINT
            if constexpr (test_deadlock)
            {
                constexpr auto deadlock = false; // NOLINT
                if constexpr (deadlock)
                {
                    // NOTE: 会死锁。
                    ex::spawn(std::move(ex::schedule(pool.get_scheduler())) |
                                  ex::then([&] noexcept {
                                      inner_calld = true;
                                      // NOTE: 没有切换线程
                                      assert(std::this_thread::get_id() == pool_id);
                                  }),
                              scope.get_token());
                }
                else
                {
                    // NOTE: 另一个线程。启动 转移sndr 可以避免死锁
                    std::jthread j{[&] {
                        ex::spawn(std::move(ex::schedule(pool.get_scheduler())) |
                                      ex::then([&] noexcept {
                                          inner_calld = true;
                                          // NOTE: 没有切换线程
                                          assert(std::this_thread::get_id() == pool_id);
                                      }),
                                  scope.get_token());
                    }};
                    j.detach();
                }
            }
            else
            {
                ex::spawn(ex::just() | ex::then([&] noexcept {
                              inner_calld = true;
                              // NOTE: 没有切换线程
                              assert(std::this_thread::get_id() == pool_id);
                          }),
                          scope.get_token());
            }
        });
        ex::spawn(ex::continues_on(ex::just(), pool.get_scheduler()) | std::move(task),
                  scope.get_token());
        while (not calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
        while (not inner_calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait inner_calld...\n";
        }
    };

    TEST("co_await async_operation 3 times 2 ") = [&] {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto task = [&]() noexcept -> ex::task<int> {
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

            assert(in == std::this_thread::get_id());

            // NOTE: 和想想的不一样。 BUG ？？
            assert(main_id == std::this_thread::get_id());
            co_return ret;
        }();
        // NOTE: 不没有死锁
#if 0
        auto [ret] = mcs::this_thread::sync_wait(
                         ex::starts_on(cpu_pool().get_scheduler(), std::move(task)))
                         .value();
#else
        auto [ret] = mcs::this_thread::sync_wait(
                         ex::on(cpu_pool().get_scheduler(), std::move(task)))
                         .value();
#endif
        // auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
        assert(ret == 6);
    };

    TEST("co_await async_operation 3 times 3 ") = [&] {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();

        auto pool_id = pool[0].thread_id();
        auto task = [&]() noexcept -> ex::task<int> {
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

            assert(in == std::this_thread::get_id());

            // NOTE: 可以外部 先 调度好，然后再执行 task。 小道儿，还是BUG
            // NOTE: 通用线程 不难 切换。
            // NOTE: 多一个线程的成本如何？ 死锁可以使用其他线程来 避免
            assert(pool_id == std::this_thread::get_id());
            co_return ret;
        }();

        auto [ret] = mcs::this_thread::sync_wait(
                         ex::continues_on(ex::just(), pool.get_scheduler()) |
                         ex::let_value([&]() { return std::move(task); }))
                         .value();
        assert(ret == 6);

        ex::spawn(std::move(task), scope.get_token());
    };

    TEST("co_await async_operation 3 times 4") = [&] {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();

        auto pool_id = pool[0].thread_id();
        auto task = [&]() noexcept -> ex::task<int> {
            std::cout << "suspend_never before...\n";

            auto in = std::this_thread::get_id();
            std::cout << "[async_operation task] start thread: " << in << '\n';

            // NOTE: 这里切换怎么样？ 是可以的
            co_await pool.get_scheduler().schedule();
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

            assert(in == main_id);
            assert(pool_id == std::this_thread::get_id());
            co_return ret;
        }();
        auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
        assert(ret == 6);
    };

#if 0 // NOTE: spawn 有BUG. 理论上应该是 总Sndr 返回是 void 就行了，发现限制确实很大了
    TEST("co_await async_operation 3 times 5") = [&] {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();

        ex::static_thread_pool<3> pool;
        auto task = [&]() noexcept -> ex::task<int> {
            int ret{};
            int times = 3;
            while (times > 0)
            {
                // NOTE: 调度就这么简单
                co_await pool[times - 1].get_scheduler().schedule();
                ret += co_await async_operation{scope, 1}; // NOLINT
                --times;
            }
            co_return ret;
        }();
        // auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
        // assert(ret == 6);
        ;
        int ret = 0;
        ex::spawn((std::move(task) | ex::then([&](int r) noexcept { ret = r; }) |
                   ex::upon_error([](auto /*e*/) {
                       std::cout << "[upon_error]:  call\n";
                       return;
                   }) |
                   ex::upon_stopped([](/*e*/) {
                       std::cout << "[upon_stopped]:  call\n";
                       return;
                   })),
                  scope.get_token());
        while (ret != 6)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
    };
#endif

#if 0 // NOTE: spawn 和  task 不难集成。 限制太多了
    TEST("co_await async_operation 3 times 5") = [&] {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();

        ex::static_thread_pool<3> pool;

        int res = 0;
        auto task = [&]() noexcept -> ex::task<> {
            int ret{};
            int times = 3;
            while (times > 0)
            {
                // NOTE: 调度就这么简单
                co_await pool[times - 1].get_scheduler().schedule();
                ret += co_await async_operation{scope, 1}; // NOLINT
                --times;
            }
            // co_return ret;
            res = ret;
        }();

        ex::spawn(std::move(task), scope.get_token());
        while (res != 6)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
    };
#endif
    mcs::this_thread::sync_wait(scope.join());

    std::cout << "main done\n";
    return 0;
}