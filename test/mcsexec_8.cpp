#include "../include/execution.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>
#include <utility>

using namespace mcs::execution;

void test_base0();
void test_base1();

int main()
{
#if 0
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
#endif
    // test_base0();
    test_base1();

    std::cout << " main done\n";
    return 0;
}
void test_base0()
{
    static_thread_pool<3> cpu_ctx;

    auto task = just(1, 2, 3) | then([](int a, int b, int c) {
                    std::cout << std::this_thread::get_id()
                              << " I am running on cpu_ctx\n";
                    return a + b + c;
                });

    auto t = starts_on(cpu_ctx.get_scheduler(), task);
    {
        using T = decltype(t.get_completion_signatures(empty_env{}));
    }
    auto [s] = mcs::this_thread::sync_wait(t).value();
    assert(s == 6);
    std::cout << "test_base0 end\n";
}
void test_base1()
{
    struct A
    {
        int v;
        A(int v) : v(v)
        {
            std::cout << v << " A()\n";
        }
        ~A()
        {
            std::cout << v << " ~A()\n";
        }

        A(const A &other) : v(other.v)
        {
            // Note: 用于提示 复制了，移动过的东西
            // Note: 多此一举，因为构造都打印了
            assert(other.v != -1);
            std::cout << v << " A(const A&)\n";
        }

        A(A &&other) noexcept : v(std::move(other.v))
        {
            other.v = -1;
            std::cout << " A(A&&)\n";
        }

        A &operator=(const A &other)
        {
            if (this != &other)
            {
                v = other.v;
                std::cout << " operator=(const A&)\n";
            }
            return *this;
        }

        A &operator=(A &&other) noexcept
        {
            if (this != &other)
            {
                v = std::move(other.v);
                other.v = -1;
                std::cout << " operator=(A&&)\n";
            }
            return *this;
        }
    };

    static_thread_pool<3> cpu_ctx;
    std::vector<A> partials = {A{1}, A{2}, A{3}, A{4}};
    for (std::size_t i; i < partials.size(); ++i)
    {
        assert(partials[i].v == (int)i);
    }

    std::cout << "task: \n";
    sender auto task =
        then(just(std::move(partials)), [](std::vector<A> &&p) -> decltype(auto) {
            std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
            return std::move(p);
        });
    std::cout << "starts_on: \n";
    {
        auto &[_, f, d] = task;
        auto &[tag, data] = d;
        data.apply([](std::vector<A> &p) {
            for (std::size_t i; i < p.size(); ++i)
            {
                assert(p[i].v == (int)i);
            }
        });
    }
    {
        // auto sch = cpu_ctx.get_scheduler();
        // Note: 不一定。因为是随机的
        // assert(sch == cpu_ctx.get_scheduler());
    }
    auto t = starts_on(cpu_ctx.get_scheduler(), std::move(task));
    {
        auto &[_, sch, task] = t;
        auto &[__, f, d] = task;
        auto &[tag, data] = d;
        data.apply([](std::vector<A> &p) {
            for (std::size_t i; i < p.size(); ++i)
            {
                assert(p[i].v == (int)i);
            }
        });
        // 复制
        {
            std::cout << "sndr = data: start\n";
            auto fun = [sndr = d]() mutable {
                auto [p] = mcs::this_thread::sync_wait(std::move(sndr)).value();
                for (std::size_t i; i < p.size(); ++i)
                {
                    assert(p[i].v == (int)i);
                }
            };
            std::cout << "sndr = data: end\n";
        }
    }
    /**
     * @brief sync_wait 前一切正常
     *
     */
    std::cout << "sync_wait: \n";
    // Note: 问题出在 sync_wait 上了
    auto [s] = mcs::this_thread::sync_wait(t).value();
}