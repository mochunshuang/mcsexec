#include "../include/execution.hpp"

#include <iostream>

using namespace mcs::execution;

int main()
{

    static_thread_pool<3> cpu_ctx;
    static_thread_pool<1> io_ctx;
    {
        scheduler auto sch1 = cpu_ctx.get_scheduler();
        scheduler auto sch2 = io_ctx.get_scheduler();

        sender auto snd1 = schedule(sch1);
        sender auto then1 = then(snd1, [] {
            std::cout << std::this_thread::get_id() << " I am running on sch1!\n";
        });
        sender auto snd2 = continues_on(then1, sch2);
        sender auto then2 = then(snd2, [] {
            std::cout << std::this_thread::get_id() << " I am running on sch2!\n";
        });

        std::cout << std::this_thread::get_id() << '\n';
        mcs::this_thread::sync_wait(then2);
        std::cout << "sync_wait done\n";
    }
    {
        sender auto task =
            schedule(cpu_ctx.get_scheduler()) //
            | then([] {
                  std::cout << std::this_thread::get_id() << " I am running on sch1!\n";
              })                                   //
            | continues_on(io_ctx.get_scheduler()) //
            | then([] {
                  std::cout << std::this_thread::get_id() << " I am running on sch2!\n";
              });
        mcs::this_thread::sync_wait(task);
        std::cout << "sync_wait done\n";
    }
    std::cout << "hello world\n";
    return 0;
}