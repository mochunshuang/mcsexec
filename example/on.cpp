#include <cstdio>
#include "../include/execution.hpp"
#include <stop_token>
#include <string>
#include <utility>
#include <iostream>

using namespace std::literals;

int main()
{
    mcs::execution::run_loop loop;

    std::jthread worker([&](std::stop_token st) {
        std::stop_callback cb{st, [&] {
                                  loop.finish();
                              }};
        loop.run();
    });

    mcs::execution::sender auto hello = mcs::execution::just("hello world"s);
    mcs::execution::sender auto print =
        std::move(hello) | mcs::execution::then([](std::string msg) {
            std::cout << "work id: " << std::this_thread::get_id() << '\n';
            return std::puts(msg.c_str());
        });

    mcs::execution::scheduler auto io_thread = loop.get_scheduler();
    mcs::execution::sender auto work = mcs::execution::on(io_thread, std::move(print));
    std::cout << "main id: " << std::this_thread::get_id() << '\n';
    auto [result] =
        mcs::this_thread::sync_wait(std::move(work) | mcs::execution::then([](auto s) {
                                        std::cout << "out work id: "
                                                  << std::this_thread::get_id() << '\n';
                                        return s;
                                    }))
            .value();

    return result;
}