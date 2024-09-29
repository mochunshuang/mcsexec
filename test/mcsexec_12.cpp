#include "../include/execution.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <thread>
#include <tuple>
#include <utility>

using namespace mcs::execution;

int main()
{
    {
        auto n = 5;
        auto f = [](int i, std::vector<double> &p) {
            std::cout << "i " << i << " call:  \n";
            p[i] = i;
            return p;
        };
        std::vector<double> partials(n);

        static_assert(std::integral<std::size_t>);

        auto task = just(std::move(partials));
        auto snd = bulk(task, n, f) | then([](std::vector<double> &&p) {
                       for (auto &i : p)
                       {
                           p[i] *= 2;
                       }
                       return std::move(p);
                   });
        {
            using T = decltype(snd.get_completion_signatures(empty_env{}));
        }
        // auto [s] = mcs::this_thread::sync_wait(snd).value();
        // for (auto &i : s)
        // {
        //     std::cout << "p[i]: " << i << " \n";
        // }
    }
    {
        //
        static_thread_pool<3> cpu_ctx;

        auto n = 5;
        auto f = [](int i, std::vector<double> &p) {
            std::cout << "i " << i << " call:  \n";
            p[i] = i;
            return p;
        };
        std::vector<double> partials(n);

        auto task = just(std::move(partials)) | then([](std::vector<double> p) {
                        std::cout << std::this_thread::get_id()
                                  << " I am running on cpu_ctx\n";
                        return std::move(p);
                    });

        auto t = starts_on(cpu_ctx.get_scheduler(), task);
        {
            auto snd = bulk(t, n, f) | then([](std::vector<double> &&p) {
                           for (auto &i : p)
                           {
                               p[i] *= 2;
                           }
                           std::cout << std::this_thread::get_id()
                                     << " I am running on cpu_ctx\n";
                           return std::move(p);
                       });
            auto [s] = mcs::this_thread::sync_wait(snd).value();
        }
    }
    std::cout << "hello world\n";
    return 0;
}