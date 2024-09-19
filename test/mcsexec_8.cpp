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
    {
        auto p =
            snd::__detail::product_type{std::forward<int &&>(1), std::forward<int &&>(2),
                                        std::forward<int &&>(3), std::forward<int &&>(4)};
        static_assert(
            std::is_same_v<decltype(std::forward<
                                    snd::__detail::product_type<int, int, int, int>>(p)),
                           snd::__detail::product_type<int, int, int, int> &&>);
        ;
    }
    auto snd_read = just(1, 2, 3) | then([](int a, int b, int c) {
                        std::cout << a << " " << b << " " << c
                                  << " thread_id: " << std::this_thread::get_id() << '\n';
                    });
    {
        mcs::this_thread::sync_wait(snd_read);
    }
    {
        auto snd_read = just(1, 2, 3, 4, 5);
        static_thread_pool<3> thread_pool;
        scheduler auto sched = thread_pool.get_scheduler();

        auto snd = starts_on(sched, snd_read);

        // decltype(snd_read) &j = snd.get<2>();
        // j.get<1>();

        using T = decltype(snd.get_completion_signatures(empty_env{}));

        auto [a, b, c, d, e] = mcs::this_thread::sync_wait(std::move(snd)).value();
        std::cout << a << " " << b << " " << c << " " << std::this_thread::get_id()
                  << '\n';
    }
    static_thread_pool<3> thread_pool;
    scheduler auto sched = thread_pool.get_scheduler();

    auto snd = starts_on(sched, snd_read);

    using T = decltype(snd.get_completion_signatures(empty_env{}));

    mcs::this_thread::sync_wait(snd);

    std::cout << "main done: thread_id: " << std::this_thread::get_id() << '\n';
    return 0;
}