#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

#include "../test_base_head.hpp"

auto &globle_counting_scope() noexcept // NOLINT
{
    static ex::counting_scope scope;
    return scope;
}

int main()
{
    ex::static_thread_pool<1> pool;
    ex::counting_scope &scope = globle_counting_scope();

    TEST("thread + on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        auto task = ex::just() | ex::then([&]() noexcept {
                        assert(std::this_thread::get_id() == pool_id);
                        calld = true;

                        // NOTE: 注意是 pool 的线程来到这这里
                    });
        ex::spawn(ex::on(pool.get_scheduler(), std::move(task)), scope.get_token());
        while (not calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
    };

    // NOTE: 死锁测试

    TEST("deadlock + on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        bool inner_calld = false;
        auto task =
            ex::just() | ex::then([&]() noexcept {
                assert(std::this_thread::get_id() == pool_id);
                calld = true;

                constexpr auto test_deadlock = true; // NOLINT
                if constexpr (test_deadlock)
                {
                    constexpr auto deadlock = false; // NOLINT
                    if constexpr (deadlock)
                    {
                        // NOTE: 来到这里就会死锁
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
                                              assert(std::this_thread::get_id() ==
                                                     pool_id);
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
        ex::spawn(ex::on(pool.get_scheduler(), std::move(task)), scope.get_token());
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

    // TODO(mcs): BUGBUG //NOTE: 实现依赖 start_on 先解决 start_on 没有bug的问题
    TEST("task + on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        auto task = [&]() -> ex::task<> {
            constexpr auto test_error = false; // NOLINT
            if constexpr (test_error)
                assert(std::this_thread::get_id() == pool_id);
            else
                assert(std::this_thread::get_id() == main_id);
            calld = true;
            co_return;
        }();
        ex::spawn(ex::on(pool.get_scheduler(), std::move(task)), scope.get_token());
        while (not calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
    };

    mcs::this_thread::sync_wait(scope.join());

    std::cout << "main done\n";
    return 0;
}