#include "../test_base_head.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

int main()
{
    static_assert(ex::snd::sender<mcs::execution::task::lazy<int>>);

    TEST(" lazy<int> is sender") = [] {
        static_assert(ex::snd::sender<mcs::execution::task::lazy<int>>);
    };

    TEST("lazy<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };

    TEST("lazy<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }() | ex::then([](int i) { return 1 + i; }));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 18);
    };

    TEST("co_await ") = [] {
        [[maybe_unused]] auto o = mcs::this_thread::sync_wait([]() -> ex::lazy<> {
            co_await ex::just(); // void // NOLINT
            std::cout << "after co_await ex::just()\n";
            [[maybe_unused]] auto v = co_await ex::just(42); // int // NOLINT
            assert(v == 42);
            [[maybe_unused]] auto [i, b, c] =
                co_await ex::just(17, true, 'c'); // tuple<int, bool, char> // NOLINT
            assert(i == 17 && b == true && c == 'c');
            try
            {
                co_await ex::just_error(-1); // exception
                assert(nullptr == "never reached");
            }
            catch (int e)
            {
                assert(e == -1);
            }
            std::cout << "about to cancel\n";
            try
            {
                co_await ex::just_stopped(); // NOLINT
            }
            catch (...) // NOLINT
            {
            } // cancel: never resumed
            assert(nullptr == "never reached");
        }());
        assert(not o);
    };

    TEST("co_await 2 ") = [] {
        auto fun = [] -> ex::lazy<int> {
            int i = 0;
            co_await ex::just(i);
            co_return -1;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_await 3 ") = [] {
        auto fun = [] -> ex::lazy<int> {
            auto [a, b] = co_await ex::just(std::make_pair(-1, "name"));
            co_return a;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_await 4 ") = [] {
        auto fun = [] -> ex::lazy<int> {
            auto [a, b] = co_await (ex::just(1) | ex::then([](auto p) noexcept {
                                        if (p > 0)
                                            return std::make_pair(-1, "name");
                                        return std::make_pair(p, "name");
                                    }));
            co_return a;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_yield only for error ") = [] {
        auto fun = [] -> ex::lazy<int> {
            co_yield mcs::execution::task::with_error{-99}; // NOLINT
            UNEXPECT("never reached");
            co_return -1;
        };
        {
            auto [ret] = mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                         std::cout
                                                             << "[co_yield]: then call\n";
                                                         std::cout << "i: " << i << '\n';
                                                         return i;
                                                     }))
                             .value();
            EXPECT(ret == -99);
        }
    };

    // Note: 目前都不支持 co_yield + while 做正常的数据处理
    TEST("with while(true)") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            int i = 100000;                                         // NOLINT
            while (true)
            {
                if (i-- == 0)
                    co_return 1;
            }
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    TEST("with while(i-->0)") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            int i = 3;                                              // NOLINT
            while (i-- > 0)
            {
                [[maybe_unused]] auto ret = co_await (
                    ex::just(i) | ex::then([](int i) noexcept {
                        std::this_thread::sleep_for(std::chrono::milliseconds(i));
                        return i;
                    }));
                std::cout << "while + co_await: " << ret << '\n';
            }
            co_return 1;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    TEST("with while(i-->0) 2 ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            int i = 3;                                              // NOLINT
            ex::static_thread_pool<3> pool;
            std::cout << "enter task thread_id: " << std::this_thread::get_id() << '\n';
            while (i-- > 0)
            {
                std::cout << "before co_await thread_id: " << std::this_thread::get_id()
                          << '\n';
                [[maybe_unused]] auto ret = co_await (
                    ex::schedule(pool[i].get_scheduler()) | ex::then([=]() noexcept {
                        std::cout
                            << "inter co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                        std::this_thread::sleep_for(std::chrono::milliseconds(i));
                        return i;
                    }));
                std::cout << "after co_await thread_id: " << std::this_thread::get_id()
                          << '\n';
            }
            co_return 1;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };
    std::cout << "\nwith while(i-->0) 2\n";
    TEST("with while(i-->0) 2 ") = [] {
        ex::static_thread_pool<1>
            start_pool; // TODO(mcs) ex::lazy 还是static_thread_pool 设计失败，调度失败
        auto start =    // NOTE: 线程体内部 总是唯一的 线程。 BUG?. 这不就是阻塞吗？？？？
            ex::schedule(start_pool.get_scheduler()) | ex::let_value([]() noexcept {
                return [] -> ex::lazy<int> { // NOLINT
                    int i = 2;               // NOLINT
                    ex::static_thread_pool<2> pool;
                    std::cout << "enter task thread_id: " << std::this_thread::get_id()
                              << '\n';
                    while (i-- > 0)
                    {
                        std::cout
                            << "before co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                        [[maybe_unused]] auto ret = co_await (
                            ex::schedule(pool[i].get_scheduler()) |
                            ex::then([=]() noexcept {
                                std::cout << "inter co_await thread_id: "
                                          << std::this_thread::get_id() << '\n';
                                std::this_thread::sleep_for(std::chrono::milliseconds(i));
                                return i;
                            }));
                        std::cout
                            << "after co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                    }
                    co_return 1;
                }();
            });
        auto rc = mcs::this_thread::sync_wait(std::move(start));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };
    std::cout << "\nwith while(i-->0) 2 replace\n";
    TEST("with while(i-->0) 2 replace ") = [] {
        ex::static_thread_pool<1> start_pool;
        auto start =
            ex::schedule(start_pool.get_scheduler()) | ex::let_value([]() noexcept {
                return [] -> ex::lazy<int> { // NOLINT
                    int i = 2;               // NOLINT
                    ex::static_thread_pool<2> pool;
                    std::cout << "enter task thread_id: " << std::this_thread::get_id()
                              << '\n';
                    while (i-- > 0)
                    {
                        std::cout
                            << "before co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                        // NOTE: 实现统一的功能。 替换 co_await
                        [[maybe_unused]] auto ret =
                            std::get<0>(mcs::this_thread::sync_wait(
                                            (ex::schedule(pool[i].get_scheduler()) |
                                             ex::then([=]() noexcept {
                                                 std::cout << "inter co_await thread_id: "
                                                           << std::this_thread::get_id()
                                                           << '\n';
                                                 std::this_thread::sleep_for(
                                                     std::chrono::milliseconds(i));
                                                 return i;
                                             })))
                                            .value());
                        std::cout
                            << "after co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                    }
                    co_return 1;
                }();
            });
        auto rc = mcs::this_thread::sync_wait(std::move(start));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    std::cout << "\nex::lazy<int> replace\n";
    TEST("ex::lazy<int> replace ") = [] {
        ex::static_thread_pool<1> start_pool;
        auto start = ex::schedule(start_pool.get_scheduler()) | ex::then([]() noexcept {
                         int i = 2; // NOLINT
                         ex::static_thread_pool<2> pool;
                         std::cout
                             << "enter task thread_id: " << std::this_thread::get_id()
                             << '\n';
                         while (i-- > 0)
                         {
                             std::cout << "before co_await thread_id: "
                                       << std::this_thread::get_id() << '\n';
                             // NOTE: 实现统一的功能。 替换 co_await + 替换sender
                             // NOTE: 不同在于 state.loop.run();
                             // 会阻塞当前线程，直到任务图，所有关联的任务全部完成
                             // NOTE: 可以确认，mutex非递归可能死锁。线程资源转移危险的
                             // NOTE: 引入了锁，效率肯定比不上  ex::lazy
                             [[maybe_unused]] auto ret = std::get<0>(
                                 mcs::this_thread::sync_wait(
                                     (ex::schedule(pool[i].get_scheduler()) |
                                      ex::then([=]() noexcept {
                                          std::cout << "inter co_await thread_id: "
                                                    << std::this_thread::get_id() << '\n';
                                          std::this_thread::sleep_for(
                                              std::chrono::milliseconds(i));
                                          return i;
                                      })))
                                     .value());
                             std::cout << "after co_await thread_id: "
                                       << std::this_thread::get_id() << '\n';
                         }
                         return 1;
                     });
        auto rc = mcs::this_thread::sync_wait(std::move(start));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    TEST("ENV") = []() {
        ex::static_thread_pool<1> start_pool;
        auto sndr = ex::schedule(start_pool.get_scheduler());
        auto env = ex::queries::get_completion_scheduler<
            ex::functional::decayed_typeof<ex::set_value>>(ex::queries::get_env(sndr));
        // NOTE: 出了能 拿 sndr 有啥用？ 拿到
        auto sndr2 = env.schedule();

        // NOTE: run_loop.get_scheduler() 拿到才行

        static_assert(std::is_same_v<decltype(sndr), decltype(sndr2)>);
        static_assert(std::is_same_v<ex::functional::decayed_typeof<ex::set_value>,
                                     ex::set_value_t>);
        // assert(sndr == sndr2);
        // NOTE: 目前确实可以添加。 语法也正确。但是没变化，想改变或许依赖定义 ex::lazy
        // 第三参数。感觉设计失败
        ex::sched::scheduler auto sched =
            ex::queries::get_scheduler(ex::queries::get_env(sndr));
        [[maybe_unused]] auto snd = ex::starts_on(sched, ex::just());
        {
            ex::sched::scheduler auto sched2 =
                ex::queries::get_scheduler(ex::queries::get_env(
                    sndr | ex::then([]() { return; }) | ex::then([]() { return 1; })));

            assert(sched == sched2);
        }
    };

    TEST("ENV") = []() {
        struct immediate_awaiter
        {
            bool await_ready() // NOLINT
            {
                return false;
            }
            auto await_suspend(std::coroutine_handle<> h) // NOLINT
            {
                return h;
            } // NOLINT
            int await_resume() // NOLINT
            {
                return 1;
            }
        };
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            // auto r = co_await immediate_awaiter{}; // TODO(mcs) 可以做到吗？
            // co_return r;
            co_return 1;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };
    return 0;
}