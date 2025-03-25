#include "../test_base_head.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

// NOLINTBEGIN
int main()
{

    TEST("base") = [scope = ex::counting_scope{}] mutable {
        ex::sender auto snd =
            ex::just() | ex::then([&] {
                //
                std::cout << "scope hello world\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // NOLINT
            });

        using T = decltype(scope.get_token());
        static_assert(ex::async_scope_token<T>);
        static_assert(ex::sender<decltype(snd)>);
        auto start = std::chrono::high_resolution_clock::now();
        try
        {
            // fire, but don't forget
            ex::spawn(std::move(snd), scope.get_token());
        }
        catch (const std::exception &e)
        {
            // 打印标准异常信息
            std::cerr << "Caught exception: " << e.what() << '\n';
        }
        catch (...)
        {
            // 处理其他未知类型的异常
            std::cerr << "Caught unknown exception" << '\n';
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        // 打印具体耗时
        std::cout << "代码块耗时: " << duration.count() << " 毫秒" << '\n';

        // 判断耗时是否小于 100 毫秒
        auto res = duration < std::chrono::milliseconds(100);
        if (res)
        {
            std::cout << "代码块耗时小于 100 毫秒。" << '\n';
        }
        else
        {
            std::cout << "代码块耗时大于或等于 100 毫秒。" << '\n';
        }

        // wait for all work nested within scope
        // to finish
        {
            using Sndr = decltype(scope.join());
            static_assert(ex::sender<Sndr>);
            using CS [[maybe_unused]] = ex::completion_signatures_of_t<Sndr>;
            using C [[maybe_unused]] =
                mcs::execution::consumers::__sync_wait::sync_wait_result_type<Sndr>;
            using Rcvr = mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>;

            using OP [[maybe_unused]] =
                decltype(ex::conn::connect(std::forward<Sndr>(std::declval<Sndr>()),
                                           std::forward<Rcvr>(std::declval<Rcvr>())));
            using n_Sndr = decltype(scope.get_token().wrap(std::declval<Sndr>()));
            using N_CS [[maybe_unused]] = ex::completion_signatures_of_t<n_Sndr>;
        }
        // mcs::this_thread::sync_wait.apply_sender(scope.join());
        mcs::this_thread::sync_wait(scope.join());

        // `ctx` is destroyed once nothing
        // references it
    };

    ex::ctx::static_thread_pool<1> pool;
    ex::counting_scope scope;
    TEST("ctx") = [&] {
        std::cout << "\ntest: ctx and scope\n";
        ex::sender auto snd =
            ex::schedule(pool.get_scheduler()) | ex::then([&] {
                //
                std::cout << "scope hello world\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // NOLINT
            });
        auto start = std::chrono::high_resolution_clock::now();
        try
        {
            // fire, but don't forget
            ex::spawn(std::move(snd), scope.get_token());
        }
        catch (const std::exception &e)
        {
            // 打印标准异常信息
            std::cerr << "Caught exception: " << e.what() << '\n';
        }
        catch (...)
        {
            // 处理其他未知类型的异常
            std::cerr << "Caught unknown exception" << '\n';
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        // 打印具体耗时
        std::cout << "代码块耗时: " << duration.count() << " 毫秒" << '\n';

        // 判断耗时是否小于 100 毫秒
        auto res = duration < std::chrono::milliseconds(100);
        if (res)
        {
            std::cout << "代码块耗时小于 100 毫秒。" << '\n';
        }
        else
        {
            std::cout << "代码块耗时大于或等于 100 毫秒。" << '\n';
        }
        EXPECT(res == true);
        mcs::this_thread::sync_wait(scope.join());
    };

    TEST("Token test") = [] {
        struct counting_scope
        {
            struct token
            {
                auto *get_scope()
                {
                    return scope;
                }

              private:
                counting_scope *scope;
                token(counting_scope *s) : scope(s) {}
                friend counting_scope;
            };
            auto get_token()
            {
                return token(this);
            }
        };
        counting_scope scope;
        auto t1 = scope.get_token();
        auto t2 = scope.get_token();
        auto t3 = t2;

        EXPECT((t1.get_scope()) == &scope);
        EXPECT((t1.get_scope()) == (t2.get_scope()));
        EXPECT((t2.get_scope()) == (t3.get_scope()));
    };

    TEST("with on") = [] {
        ex::ctx::static_thread_pool<1> pool;
        ex::counting_scope scope;
        ex::ctx::static_thread_pool<1> pool2;
        std::cout << "\ntest: with on\n";

        ex::sender auto snd =
            ex::schedule(pool.get_scheduler()) | ex::then([&] {
                //
                std::cout << "scope hello world\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // NOLINT
            });
        [[maybe_unused]] auto start = std::chrono::high_resolution_clock::now();
        try
        {
            {
                auto sched = pool2.get_scheduler();
                auto sndr = mcs::execution::factories::schedule(sched);
                auto env = sndr.get_env();
                // env.query(ex::get_scheduler);  //NOTE: 确实编译错误
                auto r = env.query(
                    ex::get_completion_scheduler_t<mcs::execution::set_value_t>{});
                EXPECT(r == sched);

                // NOTE: on 依赖 queries::get_scheduler 依赖调度。
                {
                    // 又是有了，没有JOIN集成，是没有用的
                    // NOTE: ex::spawn 缺少 queries::get_scheduler 从环境，整合失败
                    auto sndr = ex::on(pool2.get_scheduler(), std::move(snd));
                    [[maybe_unused]] auto env = ex::snd::general::SCHED_ENV(
                        ex::queries::get_completion_scheduler<
                            ex::functional::decayed_typeof<ex::set_value>>(
                            ex::queries::get_env(sndr)));
                }
            }

            //   fire, but don't forget
            ex::spawn(ex::on(pool2.get_scheduler(), std::move(snd)), scope.get_token());
        }
        catch (const std::exception &e)
        {
            // 打印标准异常信息
            std::cerr << "Caught exception: " << e.what() << '\n';
        }
        mcs::this_thread::sync_wait(scope.join());
    };

    TEST("spawn + on with parallel work") = [] {
        std::cout << "\n test: [ spawn + on with parallel work]\n";
        ex::static_thread_pool<4> pool;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [](int id) {
            return ex::just(id) | ex::then([](int id) noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       return id;
                   });
        };

        for (int i = 0; i < num; ++i)
        {
            ex::spawn(ex::on(sch, get_work(i)) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }
        auto start = std::chrono::high_resolution_clock::now();
        mcs::this_thread::sync_wait(scope.join());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "sync_wait 前后耗时: " << duration.count() << " 毫秒" << '\n';

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    TEST("spawn + on with parallel work 2 ") = [] {
        std::cout << "\n test: [ spawn + on with parallel work 2 ]\n";
        ex::static_thread_pool<4> pool;
        ex::static_thread_pool<1> pool0;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        std::array<bool, num> check_start;
        check_start.fill(false);

        auto get_work = [&](int id) {
            return ex::schedule(pool0.get_scheduler()) |
                   ex::then([=, &check_start]() noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       check_start[id] = true;
                       EXPECT(not check_done[id]);
                       return id;
                   });
        };
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num; ++i)
        {
            EXPECT(not check_done[i]);
            ex::spawn(ex::on(sch, get_work(i)) | ex::then([&](auto id) noexcept {
                          EXPECT(check_start[id]);
                          check_done[id] = true;
                      }),
                      scope.get_token());
        }

        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 前耗时: " << duration.count() << " 毫秒" << '\n';
        }
        mcs::this_thread::sync_wait(scope.join());
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 后耗时: " << duration.count() << " 毫秒" << '\n';
        }

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    TEST("spawn + starts_on with parallel work") = [] {
        std::cout << "\n test: [ spawn + starts_on with parallel work ]\n";
        ex::static_thread_pool<4> pool;
        ex::static_thread_pool<1> pool0;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [&](int id) {
            return ex::schedule(pool0.get_scheduler()) | ex::then([=]() noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       return id;
                   });
        };
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num; ++i)
        {
            ex::spawn(ex::starts_on(sch, get_work(i)) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }

        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 前耗时: " << duration.count() << " 毫秒" << '\n';
        }
        mcs::this_thread::sync_wait(scope.join());
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 后耗时: " << duration.count() << " 毫秒" << '\n';
        }

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    TEST("spawn + continues_on with parallel work") = [] {
        std::cout << "\n test: [ spawn + continues_on with parallel work ]\n";
        ex::static_thread_pool<4> pool;
        ex::static_thread_pool<1> pool0;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [&](int id) {
            return ex::schedule(pool0.get_scheduler()) | ex::then([=]() noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       return id;
                   });
        };

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num; ++i)
        {
            ex::spawn(get_work(i) | ex::continues_on(sch) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 前耗时: " << duration.count() << " 毫秒" << '\n';
        }
        mcs::this_thread::sync_wait(scope.join());
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 后耗时: " << duration.count() << " 毫秒" << '\n';
        }

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    TEST("spawn immediately ") = [] {
        std::cout << "\n test: [ spawn immediately ]\n";
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> pool0;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 1;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [&](int id) {
            return ex::schedule(pool0.get_scheduler()) | ex::then([=]() noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(100));
                       return id;
                   });
        };

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num; ++i)
        {
            ex::spawn(get_work(i) | ex::continues_on(sch) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 前耗时: " << duration.count() << " 毫秒" << '\n';
        }
        mcs::this_thread::sync_wait(scope.join());
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 后耗时: " << duration.count() << " 毫秒" << '\n';
        }

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    std::cout << " main done\n";
    return 0;
}
// NOLINTEND