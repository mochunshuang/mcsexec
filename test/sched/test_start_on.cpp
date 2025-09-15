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

int main()
{
    ex::static_thread_pool<1> pool;
    ex::counting_scope &scope = globle_counting_scope();

    TEST("thread + starts_on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        auto task = ex::just() | ex::then([&]() noexcept {
                        // NOTE: 普通snder是正确的
                        assert(std::this_thread::get_id() == pool_id);
                        calld = true;

                        // NOTE: 注意是 pool 的线程来到这这里
                    });
        ex::spawn(ex::starts_on(pool.get_scheduler(), std::move(task)),
                  scope.get_token());
        while (not calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
    };

    // NOTE: 死锁测试
    TEST("deadlock + starts_on") = [&]() {
        ex::static_thread_pool<1> pool;
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
        ex::spawn(ex::starts_on(pool.get_scheduler(), std::move(task)),
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

// TODO(mcs): BUGBUG
#if 0
    TEST("task + starts_on") = [&]() {
        [[maybe_unused]] auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool calld = false;
        auto task = [&]() -> ex::task<> {
            constexpr auto test_error = false; // NOLINT
            if constexpr (test_error)
                assert(std::this_thread::get_id() == pool_id);
            else
                assert(std::this_thread::get_id() ==
                       main_id); // NOTE: 居然不是从 pool 线程开始
            calld = true;
            co_return;
        }();
        ex::spawn(ex::starts_on(pool.get_scheduler(), std::move(task)),
                  scope.get_token());
        while (not calld)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "wait called...\n";
        }
        {
            using S = decltype(ex::starts_on(pool.get_scheduler(), std::move(task)));
            using E = decltype(std::declval<S>().get_env());
            // std::declval<E>().query(ex::queries::get_scheduler); // NOTE: 找不到
            auto sndr = ex::on(pool.get_scheduler(), std::move(task));
            // sndr.get_env().query(ex::queries::get_scheduler); // NOTE: 语法错误
        }
    };
#endif

    TEST("sndr + starts_on") = [&]() {
        auto main_id{std::this_thread::get_id()};
        auto [thread_id]{mcs::this_thread::sync_wait(
                             ex::schedule(pool.get_scheduler()) |
                             ex::then([] { return std::this_thread::get_id(); }))
                             .value_or(std::tuple{std::thread::id{}})};

        auto pool_id = thread_id;

        bool calld = false;
        auto task = ex::just() | // NOTE: 普通sndr 是符合预期的
                    ex::then([&]() {
                        calld = true;
                        EXPECT(std::this_thread::get_id() == pool_id); // NOTE: 没毛病
                    });

        mcs::this_thread::sync_wait(ex::starts_on(pool.get_scheduler(), std::move(task)));
        assert(calld);
    };
    TEST("task + starts_on 2 ") = [&]() {
        auto main_id{std::this_thread::get_id()};
        auto [thread_id]{mcs::this_thread::sync_wait(
                             ex::schedule(pool.get_scheduler()) |
                             ex::then([] { return std::this_thread::get_id(); }))
                             .value_or(std::tuple{std::thread::id{}})};

        auto pool_id = thread_id;

        bool calld = false;
        auto task = [&]() -> ex::task<> {
            if (std::this_thread::get_id() == pool_id)
            {
                std::cout << "std::this_thread::get_id() == pool_id\n";
            }
            if (std::this_thread::get_id() == main_id)
            {
                std::cout << "std::this_thread::get_id() == main_id\n";
            }
            calld = true;
            co_return;
        }();

        using T = decltype(task);
        // NOTE: 返回值是 写错局部，不是promise
        static_assert(std::is_same_v<T, ex::task<>>);

        // NOTE: task 的 sndr 只能拿到 rcvr
        mcs::this_thread::sync_wait(ex::starts_on(pool.get_scheduler(), std::move(task)));
        assert(calld);
    };
    mcs::this_thread::sync_wait(scope.join());

    std::cout << "main done\n";
    return 0;
}