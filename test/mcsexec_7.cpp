#include "../include/execution.hpp"

#include <iostream>
#include <tuple>
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
                       std::cout << a << " " << b << " " << c << " "
                                 << std::this_thread::get_id() << '\n';
                       return a;
                   });
        };
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        sender auto start = schedule(sched6);
        sender auto task = let_value(start, fun);
        auto [a] = mcs::this_thread::sync_wait(task).value();
    }

    std::cout << "hello world\n";
    return 0;
}