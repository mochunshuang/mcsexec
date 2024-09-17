#include "../include/execution.hpp"

#include <iostream>

int main()
{
    using namespace mcs::execution; // NOLINT
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        // auto s7 = schedule(sched6) | just(); // 错误
        // auto s7 = schedule(sched6); // 错误一般要配合then

        auto s7 = schedule(sched6) | then([] {});
        mcs::this_thread::sync_wait(s7);
        std::cout << "s7 done\n";
    }

    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        auto s7 = schedule(sched6);
        mcs::this_thread::sync_wait(s7);
        std::cout << "s7 done\n";
    }

    // 300000 个线程. 都没问题
    for (int i = 0; i < 100000; i++)
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        auto s7 = schedule(sched6) | then([] {});

        if (i > 0 && i % 10000 == 0)
            std::cout << "i: " << i << " thread_pool  done\n";
        mcs::this_thread::sync_wait(s7);
    }
    std::cout << "main done\n";
    return 0;
}