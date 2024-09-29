#include "../include/execution.hpp"
#include <algorithm>
#include <concepts>
#include <exception>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace mcs::execution;

template <typename...>
struct type_list
{
};
template <typename...>
struct Variant
{
};
template <typename...>
struct Tuple
{
};

int main()
{
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        // sender auto begin = just();
        // auto r = product_type<decltype(begin)>{std::move(begin)};

        sender auto s7 = schedule(sched6) //
                         | then([]() {
                               std::cout << "thread_id: " << std::this_thread::get_id()
                                         << std::endl;
                               return 42;
                           }) //
                         | then([](int i) { return i + 1; });

        using T = decltype(s7.get_completion_signatures(empty_env{}));
        // Note: then 仅仅是延续修改 set_value_t 签名
        static_assert(std::is_same_v<mcs::execution::cmplsigs::completion_signatures<
                                         mcs::execution::recv::set_value_t(int),
                                         mcs::execution::recv::set_error_t(
                                             std::__exception_ptr::exception_ptr),
                                         mcs::execution::recv::set_stopped_t()>,
                                     T>);
    }
    {
        mcs::execution::sender auto snd2 = mcs::execution::just();
        mcs::this_thread::sync_wait(std::move(snd2));
    }
    {
        auto throw_exception = []() {
            throw std::runtime_error("An error occurred!");
        };

        std::exception_ptr exception_ptr;

        try
        {
            throw_exception();
        }
        catch (...)
        {
            // 捕获异常并将其包装成 std::exception_ptr
            exception_ptr = std::make_exception_ptr(std::current_exception());
        }

        mcs::execution::sender auto snd2 = mcs::execution::just_error(exception_ptr);
        using T = decltype(snd2.get_completion_signatures(1));
        static_assert(std::is_same_v<mcs::execution::set_error_t,
                                     mcs::execution::recv::set_error_t>);
        static_assert(
            std::is_same_v<std::exception_ptr, std::__exception_ptr::exception_ptr>);

        static_assert(std::is_same_v<mcs::execution::cmplsigs::completion_signatures<
                                         mcs::execution::set_error_t(std::exception_ptr)>,
                                     T>);
        // TODO(mcs): sync_wait要求计算 value_types_of_t，没有正常的完成签名一定是失败的
        // mcs::this_thread::sync_wait(std::move(snd2));
        {
            using T = decltype(snd2.get_completion_signatures(1));
        }

        using State =
            decltype(snd2.apply([](auto &, auto &data, auto &&...) -> decltype(auto) {
                return std::forward_like<decltype(snd2)>(data);
            }));
        {
            using namespace mcs::execution;
            // tfxcmplsigs::gather_signatures<>;
            // tfxcmplsigs::gather_signatures(
            //     set_error_t, T, tfxcmplsigs::default_set_error, type_list);

            using Completions = T;
            using Tag = set_error_t;
        }
    }
    {
        mcs::execution::sender auto snd2 = mcs::execution::just_stopped();
        using T = decltype(snd2.get_completion_signatures(1));
        static_assert(
            std::is_same_v<
                mcs::execution::cmplsigs::completion_signatures<set_stopped_t()>, T>);
        // mcs::this_thread::sync_wait(std::move(snd2));
    }
    {
        mcs::execution::sender auto snd2 = mcs::execution::just(3.14, 1);
        mcs::execution::sender auto then2 =
            mcs::execution::then(std::move(snd2), [](double d, int i) {
                std::cout << "d: " << d << " i: " << i << "\n";
                return;
            });
        mcs::execution::sender auto then3 = mcs::execution::then(
            std::move(then2), []() { std::cout << "then3: " << "\n"; });

        mcs::this_thread::sync_wait(std::move(then3));
    }

    std::cout << "thread_pool: " << "\n";
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        sender auto begin = schedule(sched6);

        // sender auto begin = just();
        // auto r = product_type<decltype(begin)>{std::move(begin)};

        auto s6 = then(begin, []() { return 42; });
        auto s7 = then(s6, [](int i) { return i + 1; });
        auto [val7] = mcs::this_thread::sync_wait(s7).value();

        std::cout << val7 << '\n';
    }
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        // sender auto begin = just();
        // auto r = product_type<decltype(begin)>{std::move(begin)};

        auto s7 = schedule(sched6) //
                  | then([]() {
                        std::cout << "thread_id: " << std::this_thread::get_id()
                                  << std::endl;
                        return 42;
                    }) //
                  | then([](int i) { return i + 1; });

        auto [val7] = mcs::this_thread::sync_wait(s7).value();

        std::cout << val7 << ' ' << "thread_id: " << std::this_thread::get_id()
                  << std::endl;
    }
    {
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        auto s7 = schedule(sched6) | then([] {});
        mcs::this_thread::sync_wait(s7);
        std::cout << "s7 done\n";
    }
    {
        // TODO 一样是失败的。基础不行.
        // Note: 原因找到了，简单的无话可说。写错了。没想到吧
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        std::vector<double> partials = {0, 0, 0, 0, 0};
        auto task = just(std::move(partials)) | then([](std::vector<double> p) {
                        std::cout << std::this_thread::get_id()
                                  << " I am running on cpu_ctx\n";
                        for (std::size_t i = 0; i < p.size(); ++i)
                        {
                            p[i] = i;
                        }
                        return std::move(p);
                    });
        auto [p] = mcs::this_thread::sync_wait(task).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i] == i);
        }
    }
    // make_sender();
    std::cout << "main done\n";
    return 0;
}