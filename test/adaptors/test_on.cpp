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
    TEST("base 0") = [] {
        auto snd = ex::just() | ex::then([] {
                       std::cout << "cpu schedule id: " << std::this_thread::get_id()
                                 << "\n";
                   });
        using CS = ex::snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(
            std::is_same_v<CS,
                           mcs::execution::cmplsigs::completion_signatures<
                               mcs::execution::recv::set_value_t(),
                               mcs::execution::recv::set_error_t(std::exception_ptr)>>);
        mcs::this_thread::sync_wait(snd);
    };

    TEST("base") = [&] {
        auto snd = cpu.get_scheduler().schedule() | ex::then([] noexcept {
                       std::cout << "cpu schedule id: " << std::this_thread::get_id()
                                 << "\n";
                   });
        // NOTE: cpu.get_scheduler().schedule() 自带 exception_ptr，当
        // runloop.push_pack失败时
        // NOTE: start&() 用于是 noexcept，内部算法基于 complete算法传递 V,E,S
        // NOTE: schedule 的 start 不会传递 E
        // NOTE: start() is nest 从最内层的 start 开始，没有E通道，然后  complete
        // NOTE: complete 可能抛异常，产生 E通道
        // NOTE: std::exception_ptr 是有必要的，因为运行时 connect -> start 有可能的
        using CS = ex::snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(
            std::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                   mcs::execution::recv::set_value_t(),
                                   mcs::execution::recv::set_error_t(std::exception_ptr),
                                   mcs::execution::recv::set_stopped_t()>>);
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

    // NOTE: on 的意思是，将 sndr 给 on 传入的调度来执行。 然后回到声明ex::on算法线程
    TEST("on with 1 schedule") = [&] {
        auto on_out_id = std::this_thread::get_id();

        auto snd = ex::just() | ex::then([&] noexcept {
                       std::cout << "on with 1 schedule schedule id: "
                                 << std::this_thread::get_id() << "\n";
                       EXPECT(on_out_id != std::this_thread::get_id());
                   });
        // ex::read_env()
        auto task = ex::on(io.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([&] {
                        std::cout << "then id: " << std::this_thread::get_id() << "\n";
                        EXPECT(on_out_id == std::this_thread::get_id());
                    });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        std::cout << "test [ on with 1 schedule ] done\n\n";
    };

    TEST("on with 1 schedule 10 times with 2 schedule") = [&] {
        std::cout << "test [ on with 1 schedule 10 times with 2 schedule ] start\n\n";
        auto on_out_id = std::this_thread::get_id();

        for (int i = 0; i < 10; ++i) // NOLINT
        {
            auto snd = cpu.get_scheduler().schedule() | ex::then([&] noexcept {
                           std::cout << "on with 1 schedule schedule id: "
                                     << std::this_thread::get_id() << "\n";
                           EXPECT(on_out_id != std::this_thread::get_id());
                       });

            auto task = ex::on(io.get_scheduler(), std::move(snd)) | // NOLINT
                        ex::then([&] {
                            std::cout << "then id: " << std::this_thread::get_id()
                                      << "\n";
                            EXPECT(on_out_id == std::this_thread::get_id());
                        });
            mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        }
        std::cout << "test [ on with 1 schedule 10 times with 2 schedule ] done\n\n";
    };

    TEST("on with 1 schedule 10 times with 1 schedule") = [&] {
        std::cout << "test [ on with 1 schedule 10 times with 1 schedule ] start\n\n";
        auto on_out_id = std::this_thread::get_id();

        for (int i = 0; i < 10; ++i) // NOLINT
        {
            auto snd = ex::just() | ex::then([&] noexcept {
                           std::cout << "on with 1 schedule schedule id: "
                                     << std::this_thread::get_id() << "\n";
                           EXPECT(on_out_id != std::this_thread::get_id());
                       });

            auto task = ex::on(io.get_scheduler(), std::move(snd)) | // NOLINT
                        ex::then([&] {
                            std::cout << "then id: " << std::this_thread::get_id()
                                      << "\n";
                            EXPECT(on_out_id == std::this_thread::get_id());
                        });
            mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        }
        std::cout << "test [ on with 1 schedule 10 times with 1 schedule ] done\n\n";
    };

    // NOTE: 错误实践。类似 死锁。 自己等待自己完成才下一步，这是矛盾的。应该约束一下
    TEST("on with both before and after only 1 schedule") = [&] {
        // auto snd = io.get_scheduler().schedule() | ex::then([] noexcept {
        //                std::cout << "cpu schedule id: " << std::this_thread::get_id()
        //                          << "\n";
        //            });
        // auto task = ex::on(io.get_scheduler(), std::move(snd)) | // NOLINT
        //             ex::then([] {
        //                 std::cout << "then id: " << std::this_thread::get_id() << "\n";
        //             });
        // mcs::this_thread::sync_wait(std::move(task)); // NOLINT
        // std::cout << "test [ on with both before and after only 1 schedule ] done\n\n";
    };

    return 0;
}