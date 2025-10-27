#include "../test_base_head.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>

struct discard_all_receiver
{
    using receiver_concept = mcs::execution::receiver_t;

    template <typename... A> // NOLINTNEXTLINE
    auto set_value(A &&...a) && noexcept -> void
    {
    }

    template <typename E> // NOLINTNEXTLINE
    auto set_error(E &&e) && noexcept -> void
    {
    }

    void set_stopped() && noexcept // NOLINT
    {
    }

    constexpr auto get_env() const noexcept // NOLINT
    {

        return ex::empty_env{};
    }
};

static auto make_task() noexcept -> ex::task<int>
{
    co_return 1;
}

static auto make_task2() noexcept -> ex::task<int>
{
    co_return co_await make_task();
}

int main()
{
    // NOTE: 和 ex::lazy 不同， 协程内部 co_await 上下午切换 可以是不同的线程池
    static_assert(ex::snd::sender<mcs::execution::__task::task<int>>);

    TEST(" task<int> is sender") = [] {
        static_assert(ex::snd::sender<mcs::execution::__task::task<int>>);
        bool handle_called = false;
        auto h = [](bool &c) -> ex::task<> {
            c = true;

            // co_await std::suspend_always{}; // NOTE:外面会一直等待
            co_return;
        }(handle_called);

        std::unordered_map<int, ex::task<>> map;
        // using T = decltype(map[0]); //KEY ,value 不能移动好像
#if 0 // 不允许
        
        map.insert(std::make_pair(0, std::move(h)));
        mcs::this_thread::sync_wait(std::move(map[0]));
#endif
        mcs::this_thread::sync_wait(std::move(h));

        assert(handle_called);
    };

    TEST(" task<> connect") = [] {
        static_assert(ex::snd::sender<mcs::execution::__task::task<int>>);
        int value = 0;
        ex::static_thread_pool<1> start_pool;
        {
            auto h = [](int &c, auto &pool) -> ex::task<> {
                c = 1;
                co_await (ex::schedule(pool[0].get_scheduler()) | ex::then([&] noexcept {
                              std::this_thread::sleep_for(
                                  std::chrono::milliseconds(10)); // NOLINT
                              c = 3;
                          }));
                co_await std::suspend_always{}; // NOTE: 退出协程。 释放线程资源
                co_return;
            }(value, start_pool);

            auto op = ex::connect(std::move(h), discard_all_receiver{});

            // NOTE: OP 是堆内存的化，释放掉，就不可能内存泄漏
            assert(value == 0);
            op.start(); // NOTE:当封装到一个类型，用堆内存生成对象，就能紧急启动、
            assert(value == 1);
            while (value == 1) // NOLINT
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                std::cout << "   >>> value==1\n";
            }
            assert(value == 3);
        }
        // NOTE: start_pool 的线程不会被阻塞
        {
            value = 0;
            auto h = [](int &c, auto &pool) -> ex::task<> {
                c = 1;
                co_await (ex::schedule(pool[0].get_scheduler()) | ex::then([&] noexcept {
                              std::this_thread::sleep_for(
                                  std::chrono::milliseconds(10)); // NOLINT
                              c = 3;
                          }));
                co_await std::suspend_always{}; // NOTE: 退出协程
                co_return;
            }(value, start_pool);

            // auto op = ex::connect(std::move(h), discard_all_receiver{});
            using op_type = decltype(ex::connect(std::move(h), discard_all_receiver{}));

            struct operation_type
            {
                explicit operation_type(ex::task<> &&h)
                    : op{ex::connect(std::move(h), discard_all_receiver{})}, self{this}
                {
                    op.start(); // 紧急启动
                }
                op_type op;
                operation_type *self;
            };

            assert(value == 0);
            // NOTE:当封装到一个类型，用堆内存生成对象，就能紧急启动、
            operation_type op{std::move(h)};
            assert(value == 1);
            while (value == 1) // NOLINT
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                std::cout << "   >>> operation_type value==1\n";
            }
            assert(value == 3);
        }
    };

    TEST("task<int> and task<int> ") = [] {
        auto handle = [] -> ex::task<int> { // NOLINT
            co_return 17;                   // NOLINT
        };
        auto rc = mcs::this_thread::sync_wait([&] -> ex::task<int> { // NOLINT
            co_return co_await handle();                             // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };

    TEST("task<variant> and task<int> ") = [] {
        auto handle = [] -> ex::task<std::variant<int, std::error_code>> { // NOLINT
            co_return std::make_error_code(std::errc::connection_reset);
            co_return 17; // NOLINT
        };
        auto rc = mcs::this_thread::sync_wait([&] -> ex::task<int> { // NOLINT
            auto var = co_await handle();
            if (std::holds_alternative<std::error_code>(var))
                co_return -1;
            co_return std::get<int>(var); // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == -1);
    };

    TEST("task<int> and task<int>_with_except ") = [] {
        // NOLINTBEGIN
        class Error
        {
          private:
            std::variant<int, std::exception_ptr, std::error_code> value;

          public:
            // 构造函数
            Error(int code) : value(code) {}
            Error(std::exception_ptr &&ex) : value(std::move(ex)) {}
            Error(std::error_code &&ec) : value(std::move(ec)) {}

            // 判断是否包含异常
            bool has_exception() const
            {
                return std::holds_alternative<std::exception_ptr>(value);
            }

            auto index() const noexcept
            {
                return value.index();
            }

            // 获取异常指针（需配合 has_exception 使用）
            std::exception_ptr exception() const
            {
                return std::get<std::exception_ptr>(value);
            }
        };

        auto handle = [] -> ex::task<int> { // NOLINT
            throw std::runtime_error{"error"};
            co_return 17; // NOLINT
        };
        using S =
            decltype(handle() | ex::then([](int v) { return Error{std::move(v)}; }) |
                     ex::upon_error(
                         [](auto &&e) noexcept { return Error{std::move(e)}; }));
        using CS = ex::completion_signatures_of_t<S>; // NOTE: 组合会修改签名
        static_assert(
            std::is_same_v<
                ex::completion_signatures<ex::set_value_t(Error), ex::set_stopped_t()>,
                CS>);

        // NOLINTEND
        auto rc = mcs::this_thread::sync_wait([&] -> ex::task<int> { // NOLIN
            Error v =
                co_await (handle() | ex::then([](int v) { return Error{std::move(v)}; }) |
                          ex::upon_error([](auto &&e) noexcept {
                              return Error{std::move(e)};
                          })); // NOLINT

            EXPECT(v.index() == 1);
            co_return 17;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };

    {
        static int count = 0;
        struct test_object
        {
            test_object() noexcept
            {
                count++;
            };
            test_object(test_object &&o) noexcept
                : valid(std::exchange(o.valid, false)) {};
            test_object &operator=(test_object &&) = delete;
            test_object(const test_object &o) = delete;
            test_object &operator=(const test_object &) = delete;
            ~test_object() noexcept
            {
                std::cout << "--->>> Resource destroyed\n";
                valid = false;
                count--;
            }
            bool valid{true}; // NOLINT
        };
        TEST("task<variant> and && ") = [] {
            ex::static_thread_pool<1> start_pool;
            auto handle = [&](test_object &&obj) -> ex::task<bool> { // NOLINT
                auto input_id = std::this_thread::get_id();
                auto v = co_await (ex::schedule(start_pool[0].get_scheduler()) |
                                   ex::then([&] noexcept {
                                       std::this_thread::sleep_for(
                                           std::chrono::milliseconds(50)); // NOLINT
                                       return 1;
                                   }));
                EXPECT(v == 1);
                EXPECT(input_id != std::this_thread::get_id());

                // NOTE: 纯右值保证 1 次构造，1 次析构。 没有未定义行为
                // NOTE: 可以替代 完美转发
                EXPECT(count == 1);
                std::cout << "--->>> co_return obj.valid\n";
                co_return obj.valid; // NOLINT
            };
            auto rc = mcs::this_thread::sync_wait(
                [&](test_object &&obj) -> ex::task<bool> {     // NOLINT
                    co_return co_await handle(std::move(obj)); // NOLINT
                }(test_object{}));
            assert(rc);
            auto [value] = rc.value();
            EXPECT(value == true);

            EXPECT(count == 0);
        };
    }

    {
        TEST("change pool test ") = [] {
            ex::static_thread_pool<1> start_pool;
            ex::static_thread_pool<1> pool;
            auto start =
                ex::schedule(start_pool.get_scheduler()) | ex::let_value([&]() noexcept {
                    return [](auto &start_pool, auto &pool) -> ex::task<int> { // NOLINT
                        std::cout
                            << "enter task thread_id: " << std::this_thread::get_id()
                            << '\n';
                        std::cout
                            << "before co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                        assert(start_pool[0]
                                   .is_waiting_task_state()); // NOTE: 还在执行任务中
                        [[maybe_unused]] auto ret = co_await (
                            ex::schedule(pool[0].get_scheduler()) |
                            ex::then([&]() noexcept {
                                // NOTE: start_pool 已经执行完了所有任务. 已经切换了线程
                                assert(start_pool[0].is_waiting_task_state());
                                std::cout << "inter co_await thread_id: "
                                          << std::this_thread::get_id() << '\n';
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                return 0;
                            }));
                        std::cout
                            << "after co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                        co_return 1;
                    }(start_pool, pool);
                });
            auto rc = mcs::this_thread::sync_wait(std::move(start));
            assert(rc);
            auto [value] = rc.value_or(std::tuple{0});
            EXPECT(value == 1);

            std::cout << "change pool test done" << '\n' << '\n';
        };
    }

    TEST("task<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::task<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };

    TEST("task<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::task<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }() | ex::then([](int i) { return 1 + i; }));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 18);
    };

    TEST("co_await ") = [] {
        [[maybe_unused]] auto o = mcs::this_thread::sync_wait([]() -> ex::task<> {
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
        auto fun = [] -> ex::task<int> {
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
        auto fun = [] -> ex::task<int> {
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
        auto fun = [] -> ex::task<int> {
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

    // NOTE: 暂时觉得没必要
#if false // NOLINT
    TEST("co_yield only for error ") = [] {
        auto fun = [] -> ex::task<int> {
            co_yield mcs::execution::__task::with_error{-99}; // NOLINT
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
#endif

    // Note: 目前都不支持 co_yield + while 做正常的数据处理
    TEST("with while(true)") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::task<int> { // NOLINT
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
        auto rc = mcs::this_thread::sync_wait([] -> ex::task<int> { // NOLINT
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

    ex::static_thread_pool<3> pool;

    struct single_thread_context
    {
      private:
        ex::run_loop loop;
        ::std::thread thread{&single_thread_context::run, this};

        static auto run(single_thread_context *self) -> void
        {
            self->loop.run();
        }

      public:
        single_thread_context() = default;
        ~single_thread_context()
        {
            this->finish();
            this->thread.join();
        }
        auto get_scheduler()
        {
            return this->loop.get_scheduler();
        }
        void finish()
        {
            this->loop.finish();
        }
    };
    single_thread_context context;
    TEST("with while(i-->0) ") = [&] {
        auto rc = mcs::this_thread::sync_wait([](auto &pool) -> ex::task<int> { // NOLINT
            int i = 3;                                                          // NOLINT

            std::cout << "enter task thread_id: " << std::this_thread::get_id() << '\n';
            while (i-- > 0)
            {
                std::cout << "before co_await thread_id: " << std::this_thread::get_id()
                          << '\n';
                [[maybe_unused]] auto ret = co_await (
                    ex::schedule(pool.get_scheduler()) | ex::then([=]() noexcept {
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
        }(context));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };
    TEST("with while(i-->0) 2 ") = [&] {
        auto rc = mcs::this_thread::sync_wait([](auto &pool) -> ex::task<int> { // NOLINT
            int i = 3;                                                          // NOLINT

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
        }(pool));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };
    std::cout << "\nwith while(i-->0) 2\n";
    TEST("with while(i-->0) 2 ") = [] {
        ex::static_thread_pool<1> start_pool;
        ex::static_thread_pool<2> pool;
        auto start = // NOTE: 线程体内部 总是唯一的 线程。 BUG?. 这不就是阻塞吗？？？？
            ex::schedule(start_pool.get_scheduler()) | ex::let_value([&]() noexcept {
                return [](auto &pool) -> ex::task<int> { // NOLINT
                    int i = 2;                           // NOLINT

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
                }(pool);
            });
        auto rc = mcs::this_thread::sync_wait(std::move(start));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };
    std::cout << "\nwith while(i-->0) 2 replace\n";
    TEST("with while(i-->0) 2 replace ") = [] {
        ex::static_thread_pool<1> start_pool;
        ex::static_thread_pool<2> pool;
        auto start =
            ex::schedule(start_pool.get_scheduler()) | ex::let_value([&]() noexcept {
                return [](auto &pool) -> ex::task<int> { // NOLINT
                    int i = 2;                           // NOLINT

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
                }(pool);
            });
        auto rc = mcs::this_thread::sync_wait(std::move(start));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    std::cout << "\nex::task<int> replace\n";
    TEST("ex::task<int> replace ") = [] {
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
        // NOTE: get_completion_scheduler 和  get_scheduler 是不一样的。 Q的tag不一样
        ex::sched::scheduler auto sched =
            ex::queries::get_completion_scheduler<ex::set_value_t>(
                ex::queries::get_env(sndr));
        [[maybe_unused]] auto snd = ex::starts_on(sched, ex::just());
        {
            ex::sched::scheduler auto sched2 =
                ex::queries::get_completion_scheduler<ex::set_value_t>(
                    ex::queries::get_env(sndr | ex::then([]() { return; }) |
                                         ex::then([]() { return 1; })));

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
        auto rc = mcs::this_thread::sync_wait([] -> ex::task<int> { // NOLINT
            auto r = co_await immediate_awaiter{}; // TODO(mcs) 可以做到吗？
            co_return r;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    TEST("co_yield only for error ") = [] {
        auto fun = [] -> ex::task<int> {
            // 感觉没必要了
            // co_yield mcs::execution::task::with_error{-99}; // NOLINT

            // throw std::error_code{-99}; //NOTE: 不允许
            throw std::runtime_error{"error"};
            UNEXPECT("never reached");
            co_return -1;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_yield]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }) |
                                            ex::upon_error([](auto /*e*/) {
                                                std::cout << "[upon_error]:  call\n";
                                                return -1;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_yield only for error with  noexcept") = [] {
        auto fun = []() noexcept -> ex::task<int> {
            // 感觉没必要了
            // co_yield mcs::execution::task::with_error{-99}; // NOLINT

            // throw std::error_code{-99}; //NOTE: 不允许
            throw std::runtime_error{"error"};
            UNEXPECT("never reached");
            co_return 0;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_yield]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }) |
                                            ex::upon_error([](auto /*e*/) {
                                                std::cout << "[upon_error]:  call\n";
                                                return -1;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };
    TEST("co_await ex::task") = [] {
        auto task = []() noexcept -> ex::task<int> {
            co_return co_await []() noexcept -> ex::task<int> {
                co_return 1;
            }();
        }();
        auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
        EXPECT(ret == 1);

        {
            auto task = []() noexcept -> ex::task<int> {
                auto ret = co_await []() noexcept -> ex::task<int> {
                    co_return 1;
                }();
                co_return ret;
            }();
            auto [ret] = mcs::this_thread::sync_wait(std::move(task)).value();
            EXPECT(ret == 1);
        }
        {
            auto [ret] = mcs::this_thread::sync_wait(make_task2()).value();
            EXPECT(ret == 1);
        }
    };

    return 0;
}