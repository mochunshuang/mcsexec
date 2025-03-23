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
        EXPECT(res == false);

        // wait for all work nested within scope
        // to finish
        {
            using Sndr = decltype(scope.join());
            static_assert(ex::sender<Sndr>);
            using CS [[maybe_unused]] = ex::completion_signatures_of_t<Sndr>;
            using C [[maybe_unused]] =
                mcs::execution::consumers::__sync_wait::sync_wait_result_type<Sndr>;
            using Rcvr = mcs::execution::consumers::__sync_wait::sync_wait_receiver<
                mcs::execution::snd::__detail::basic_sender<
                    mcs::execution::scope::counting_scope::join_t,
                    mcs::execution::scope::counting_scope *>>;
            static_assert(
                std::is_same_v<
                    Rcvr,
                    mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>>);
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

    std::cout << " main done\n";
    return 0;
}
// NOLINTEND