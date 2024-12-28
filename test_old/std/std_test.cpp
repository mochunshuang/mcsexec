#include "../../include/execution.hpp"
#include <cstdio>
#include <execution>
#include <string>
#include <thread>
#include <utility>
using namespace std::literals;

int main()
{
    namespace stdx = mcs::execution;
    stdx::run_loop loop;

    std::jthread worker([&](std::stop_token st) {
        std::stop_callback cb{st, [&] {
                                  loop.finish();
                              }};
        loop.run();
    });

    stdx::sender auto hello = stdx::just("hello world"s);
    stdx::sender auto print = std::move(hello) | stdx::then([](std::string msg) {
                                  return std::puts(msg.c_str());
                              });

    stdx::scheduler auto io_thread = loop.get_scheduler();
    stdx::sender auto work = stdx::on(io_thread, std::move(print));

    // auto [result] = std::this_thread::sync_wait(std::move(work)).value();
    auto [result] = mcs::this_thread::sync_wait(std::move(work)).value();

    return result;
}