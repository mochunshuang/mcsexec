#include "../test_base_head.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

int main()
{
    ex::static_thread_pool<1> io;
    ex::static_thread_pool<1> cpu;
    io.printInfo();
    cpu.printInfo();
    std::cout << "main id: " << std::this_thread::get_id() << "\n";

    TEST("base") = [&] {
        auto snd = cpu.get_scheduler().schedule() | ex::then([] {
                       std::cout << "cpu schedule id: " << std::this_thread::get_id()
                                 << "\n";
                   });
        mcs::this_thread::sync_wait(snd);
        auto task = ex::on(io.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([] {
                        std::cout << "then id: " << std::this_thread::get_id() << "\n";
                    });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        std::cout << "test done\n\n";
    };

    TEST("base 2") = [&] {
        auto snd =
            cpu.get_scheduler().schedule() | ex::then([&] {
                std::cout << "cpu schedule id: " << std::this_thread::get_id() << "\n";
                auto task = ex::on(io.get_scheduler(),
                                   ex::just()) | // NOLINT
                            ex::then([] {
                                std::cout << "then id: " << std::this_thread::get_id()
                                          << "\n";
                            });
                mcs::this_thread::sync_wait(std::move(task)); // NOLINT
            });
        mcs::this_thread::sync_wait(snd);
        std::cout << "test done\n\n";
    };

    TEST("on and starts_on") = [&] {
        auto snd = cpu.get_scheduler().schedule() | ex::then([] {
                       std::cout << "cpu schedule id: " << std::this_thread::get_id()
                                 << "\n";
                   });
        auto task = ex::starts_on(io.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([] {
                        std::cout << "then id: " << std::this_thread::get_id() << "\n";
                    });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        std::cout << "test done\n\n";
    };
    TEST("on and continues_on") = [&] {
        auto snd = cpu.get_scheduler().schedule() | ex::then([] {
                       std::cout << "cpu schedule id: " << std::this_thread::get_id()
                                 << "\n";
                   });
        auto task = std::move(snd) | ex::continues_on(io.get_scheduler()) | // NOLINT
                    ex::then([] {
                        std::cout << "then id: " << std::this_thread::get_id() << "\n";
                    });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        std::cout << "test done\n\n";
    };
    return 0;
}