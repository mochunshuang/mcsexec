
#include <algorithm>
#include <cassert>
#include <iostream>
#include "../../include/execution.hpp"

using namespace mcs::execution; // NOLINT
int main()
{
    {
        // auto snd = just() | then([] { //
        //                std::cout << "then call \n";
        //            });
        // mcs::this_thread::sync_wait(snd);
    }
#if 0
    {
        static_thread_pool<3> thread_pool;
    }
    std::cout << "thread_pool RAII OK \n";
    {
        static_thread_pool<3> thread_pool;

        scheduler auto sched = thread_pool.get_scheduler();
        auto snd0 = just() | then([] { //
                        std::cout << "then call \n";
                    });
        auto snd = starts_on(sched, snd0);

        mcs::this_thread::sync_wait(snd);
    }
    std::cout << "test 1 OK \n";
    {
        for (int i = 0; i < 100000; i++) // NOLINT
        {
            auto snd = just() | then([] { //
                                          //    std::cout << "then call \n";
                       });

            if (i > 0 && i % 10000 == 0) // NOLINT
                std::cout << "i: " << i << " thread_pool  done\n";
            mcs::this_thread::sync_wait(snd);
        }
    }
#endif
    {
        static_thread_pool<3> thread_pool;
        for (int i = 0; i < 10000; i++) // NOLINT
        {
            auto snd0 = just() | then([] { //
                            // TODO(mcs): 为何依赖  std::cout << " ";
                            std::cout << " ";
                        });
            auto snd = starts_on(thread_pool.get_scheduler(), snd0);

            if (i > 0 && i % 1000 == 0) // NOLINT
                std::cout << "i: " << i << " thread_pool  done\n";
            mcs::this_thread::sync_wait(snd);
        }
        {
            static_thread_pool<3> thread_pool;
            for (int i = 0; i < 100000; i++) // NOLINT
            {
                auto snd0 = just() | then([] { //
                                // Note: 原因是 for 循环优化. move 即可 不需要 std::cout
                            });
                auto snd = starts_on(thread_pool.get_scheduler(), snd0);

                if (i > 0 && i % 10000 == 0) // NOLINT
                    std::cout << "i: " << i << " thread_pool  done\n";
                mcs::this_thread::sync_wait(std::move(snd));
            }
        }
    }
    std::cout << "\nbase done\n";
    {
        int v = 0;
        static_thread_pool<3> thread_pool;

        auto snd0 = just() | then([&] { //
                        v = 1;
                    });
        auto snd = starts_on(thread_pool.get_scheduler(), snd0);
        mcs::this_thread::sync_wait(std::move(snd));
        assert(v == 1);
    }
    {
        int v = 0;
        static_thread_pool<3> thread_pool;
        for (int i = 0; i < 10000; i++) // NOLINT
        {
            auto snd0 = just() | then([&] { //
                            v = 1;
                        });
            auto snd = starts_on(thread_pool.get_scheduler(), snd0);
            mcs::this_thread::sync_wait(std::move(snd));
            assert(v == 1);
        }
    }
    std::cout << "\nmain done\n";
    return 0;
}