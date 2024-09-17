partial implementation of the *Senders* model of asynchronous programming proposed by [**P2300 - `std::execution`**](http://wg21.link/p2300)

For the sole purpose of learning c++ templates, without any suggestion or hint

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

