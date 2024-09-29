#include "../include/execution.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#include <tuple>
#include <utility>

using namespace mcs::execution;

int main()
{
    static_thread_pool<3> cpu_ctx;
    {
        auto snd = cpu_ctx.get_scheduler().schedule();
        auto task = snd | then([]() {
                        std::cout << std::this_thread::get_id()
                                  << " I am running on cpu_ctx!\n";
                        return 1;
                    });
        using T [[maybe_unused]] = decltype(task.get_completion_signatures(empty_env{}));
        auto [a] = mcs::this_thread::sync_wait(task).value();
        assert(a == 1);
    }
    {
        auto task = just() | then([]() {
                        std::cout << std::this_thread::get_id()
                                  << " I am running on cpu_ctx!\n";
                    });
        [[maybe_unused]] auto t = on(cpu_ctx.get_scheduler(), task) | then([]() {
                                      std::cout << std::this_thread::get_id()
                                                << " I am running on main thead!\n";
                                  });

        mcs::this_thread::sync_wait(t);
    }
    std::cout << std::this_thread::get_id() << " I am running on main()\n";
    return 0;
}