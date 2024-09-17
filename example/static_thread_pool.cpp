
#include <iostream>
#include "../include/execution.hpp"

int main()
{
    using namespace mcs::execution;
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
    std::cout << "hello world\n";
    return 0;
}