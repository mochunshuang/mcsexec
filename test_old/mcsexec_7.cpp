#include "../include/execution.hpp"

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>

using namespace mcs::execution;

int main()
{
    auto fun = []() -> sender auto {
        return just(1) | then([](int a) {
                   std::cout << "then: " << a << "\n";
                   return a;
               });
    };

    static_thread_pool<3> thread_pool;
    scheduler auto sched6 = thread_pool.get_scheduler();
    sender auto start = schedule(sched6);
    sender auto task = let_value(start, fun);
    auto [a] = mcs::this_thread::sync_wait(task).value();
    std::cout << "sync_wait: " << a << "\n";

    {
        auto fun = []() -> sender auto {
            return just(1, 2, 3) | then([](int a, int b, int c) {
                       std::cout << a << " " << b << " " << c
                                 << " id: " << std::this_thread::get_id() << '\n';
                       return a;
                   });
        };
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        sender auto task = let_value(schedule(sched6), fun);
        auto [a] = mcs::this_thread::sync_wait(task).value();
        assert(a == 1);
    }
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        std::vector<double> partials(5);
        auto fun = []() -> sender auto {
            return just(1, 2, 3) | then([](int a, int b, int c) {
                       std::cout << a << " " << b << " " << c
                                 << " id: " << std::this_thread::get_id() << '\n';
                       return a;
                   });
        };

        sender auto task = let_value(schedule(sched6), fun);
        using T0 = decltype(schedule(sched6).get_completion_signatures(empty_env{}));
        using T1 = decltype(fun().get_completion_signatures(empty_env{}));
        using T = decltype(task.get_completion_signatures(empty_env{}));
        auto [a] = mcs::this_thread::sync_wait(task).value();
    }
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        std::vector<double> partials = {0, 0, 0, 0, 0};
        auto fun = [&]() -> sender auto {
            return just(std::move(partials)) | then([](std::vector<double> &&p) {
                       std::cout << std::this_thread::get_id()
                                 << " I am running on cpu_ctx\n";
                       for (std::size_t i = 0; i < p.size(); ++i)
                       {
                           p[i] = i;
                       }
                       return std::move(p);
                   });
            ;
        };
        sender auto task = let_value(schedule(sched6), fun);
        auto [p] = mcs::this_thread::sync_wait(task).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i] == i);
        }
    }

    std::cout << "hello world\n";
    return 0;
}