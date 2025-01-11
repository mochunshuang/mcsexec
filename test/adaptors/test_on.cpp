#include "../test_base_head.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

int main()
{
    TEST("base") = [] {
        ex::static_thread_pool<1> io;
        ex::static_thread_pool<1> cpu;
        auto snd = cpu.get_scheduler().schedule() | ex::then([] {
                       std::cout << "id: " << std::this_thread::get_id() << "\n";
                   });
        mcs::this_thread::sync_wait(snd);
        auto task =
            ex::on(io.get_scheduler(), std::move(snd)) | // NOLINT
            ex::then([] { std::cout << "id: " << std::this_thread::get_id() << "\n"; });
        std::cout << "main id: " << std::this_thread::get_id() << "\n";
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
    };

    return 0;
}