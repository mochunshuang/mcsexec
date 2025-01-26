partial implementation of the *Senders* model of asynchronous programming proposed by [**P2300 - `std::execution`**](http://wg21.link/p2300)

For the sole purpose of learning c++ templates, without any suggestion or hint

[![Build Status](https://github.com/your-username/your-repo/actions/workflows/cmake.yml/badge.svg)](https://github.com/your-username/your-repo/actions/workflows/cmake.yml)
[![Codecov](https://codecov.io/gh/your-username/your-repo/branch/main/graph/badge.svg)](https://codecov.io/gh/your-username/your-repo)
[![CMake Version](https://img.shields.io/badge/CMake-3.30.0-blue)](https://cmake.org/)
[![G++ Version](https://img.shields.io/badge/G++-14.2-green)](https://gcc.gnu.org/)

## Features

- CMake 3.30
- G++ 14.2
- Code coverage with `gcov` and `lcov`
- Automated testing with GitHub Actions

**example**

~~~c++
#include <iostream>
#include "../include/execution.hpp"

int main()
{
    mcs::execution::sender auto j = mcs::execution::just(3.14, 1);
    mcs::execution::sender auto t =
        mcs::execution::then(std::move(j), [](double d, int i) {
            std::cout << "d: " << d << " i: " << i << "\n";
            return;
        });
    mcs::execution::sender auto t2 =
        mcs::execution::then(std::move(t), []() { std::cout << "then3: " << "\n"; });

    mcs::this_thread::sync_wait(std::move(t2));

    {
        using namespace mcs::execution;
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        sender auto begin = schedule(sched6);

        auto t0 = then(begin, []() { return 42; });
        auto t1 = then(t0, [](int i) { return i + 1; });
        auto [v] = mcs::this_thread::sync_wait(t1).value();

        std::cout << v << '\n';
    }

    {
        using namespace mcs::execution;
        static_thread_pool<3> thread_pool;
        scheduler auto sched = thread_pool.get_scheduler();

        auto s = schedule(sched) //
                 | then([]() {
                       std::cout << "thread_id: " << std::this_thread::get_id()
                                 << std::endl;
                       return 42;
                   }) //
                 | then([](int i) { return i + 1; });

        auto [val] = mcs::this_thread::sync_wait(s).value();

        std::cout << val << ' ' << "thread_id: " << std::this_thread::get_id()
                  << std::endl;
    }
    return 0;
}
~~~

