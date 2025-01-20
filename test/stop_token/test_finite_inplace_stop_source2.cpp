
#include <iostream>
#include <version>
#include <cstdint>
#include <utility>
#include <atomic>
#include <thread>
#include <cassert>

#include <xmmintrin.h>
// NOLINTBEGIN
#include <chrono>
// #include <print>
#include <algorithm>
#include <numeric>
#include <vector>

// Define some helper functions
#include "../../include/execution.hpp"

constexpr std::uint32_t iteration_count = 100'000;
constexpr std::uint32_t pass_count = 20;

template <typename F>
void timed_invoke(const char *label, F f)
{
    auto start = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    auto time = (end - start);
    auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();

    // std::print("{} took {: >4}.{:03}us\n", label, time_ns / 1000, time_ns % 1000);
    std::cout << label << " took " << std::setw(4) << std::setfill(' ')
              << (time_ns / 1000) << "." << std::setw(3) << std::setfill('0')
              << (time_ns % 1000) << "us\n";
}

void print_times(const char *label, std::vector<std::chrono::nanoseconds> &times)
{
    std::sort(times.begin(), times.end());

    auto min_time_ns = times.front().count();
    auto max_time_ns = times.back().count();
    auto total_time =
        std::accumulate(times.begin(), times.end(), std::chrono::nanoseconds{});
    auto avg_time = total_time / times.size();
    auto avg_time_ns = avg_time.count();
    auto p50_time = ((times.size() % 2) == 0 && (times.size() >= 2))
                        ? (times[times.size() / 2] + times[times.size() / 2 + 1]) / 2
                        : times[times.size() / 2];
    auto p50_time_ns = p50_time.count();

    // std::print(
    //     "{} {: >4}.{:03} - {: >4}.{:03}us (avg {: >4}.{:03}us, p50 {: >4}.{:03}us)\n",
    //     label, min_time_ns / 1000, min_time_ns % 1000, max_time_ns / 1000,
    //     max_time_ns % 1000, avg_time_ns / 1000, avg_time_ns % 1000, p50_time_ns / 1000,
    //     p50_time_ns % 1000);
    std::cout << label << " " << std::setw(4) << std::setfill(' ') << (min_time_ns / 1000)
              << "." << std::setw(3) << std::setfill('0') << (min_time_ns % 1000) << " - "
              << std::setw(4) << std::setfill(' ') << (max_time_ns / 1000) << "."
              << std::setw(3) << std::setfill('0') << (max_time_ns % 1000) << "us (avg "
              << std::setw(4) << std::setfill(' ') << (avg_time_ns / 1000) << "."
              << std::setw(3) << std::setfill('0') << (avg_time_ns % 1000) << "us, p50 "
              << std::setw(4) << std::setfill(' ') << (p50_time_ns / 1000) << "."
              << std::setw(3) << std::setfill('0') << (p50_time_ns % 1000) << "us)\n";
}

template <typename F>
void timed_invoke_multi(const char *label, std::uint32_t count, F f)
{
    if (count == 0)
        return;

    std::vector<std::chrono::nanoseconds> times;
    times.reserve(count);

    for (std::uint32_t i = 0; i < count; ++i)
    {
        auto start = std::chrono::steady_clock::now();
        f();
        auto end = std::chrono::steady_clock::now();
        times.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
    }

    print_times(label, times);
}

//
// Single-Thread Regsiter/Unregister Callback
//

void single_thread_register_unregister_1()
{
    mcs::execution::inplace_stop_source ss;
    auto cb = [x = 1] noexcept {
    };
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        mcs::execution::inplace_stop_callback scb{ss.get_token(), cb};
    }
}

void single_thread_register_unregister_2()
{
    mcs::execution::single_inplace_stop_source ss;
    auto cb = [x = 1] noexcept {
    };
    static_assert(
        mcs::execution::stoptoken::__detail::invocable_destructible<decltype(cb)>);

    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        mcs::execution::single_inplace_stop_callback scb{ss.get_token(), cb};
    }
}

//
// Single-Thread No Callback + request_stop
//

void single_thread_no_callback_stop_1()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        mcs::execution::inplace_stop_source ss;
        ss.request_stop();
    }
}

template <std::size_t MaxCallbacks>
void single_thread_no_callback_stop_2()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::array<mcs::execution::single_inplace_stop_source, MaxCallbacks> ss;
        for (auto &s : ss)
        {
            s.request_stop();
        }
    }
}

template <std::size_t MaxCallbacks>
void single_thread_no_callback_stop_3()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        mcs::execution::finite_inplace_stop_source<MaxCallbacks> ss;
        ss.request_stop();
    }
}

//
// Single-Thread Register/Unregister Nx Callback + request_stop
//

template <std::size_t CallbackCount>
void single_thread_register_multiple_with_stop_1()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        mcs::execution::inplace_stop_source ss;
        std::size_t count = 0;
        auto cb = [&] {
            ++count;
        };

        using stop_callback_t = mcs::execution::inplace_stop_callback<decltype(cb)>;

        auto cbs = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<stop_callback_t, CallbackCount>{
                ((void)Is, stop_callback_t{ss.get_token(), cb})...};
        }(std::make_index_sequence<CallbackCount>{});

        ss.request_stop();

        if (count != CallbackCount)
        {
            std::terminate();
        }
    }
}

template <std::size_t Count, std::size_t CallbackCount>
void single_thread_register_multiple_with_stop_2()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::array<mcs::execution::single_inplace_stop_source, Count> ss;
        std::size_t count = 0;
        auto cb = [&] {
            ++count;
        };

        using stop_callback_t =
            mcs::execution::single_inplace_stop_callback<decltype(cb)>;

        auto cbs = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<stop_callback_t, CallbackCount>{
                stop_callback_t{ss[Is].get_token(), cb}...};
        }(std::make_index_sequence<CallbackCount>{});

        for (std::size_t j = 0; j < Count; ++j)
        {
            ss[j].request_stop();
        }

        if (count != CallbackCount)
        {
            std::terminate();
        }
    }
}

template <std::size_t Count, typename CB, std::size_t... Is>
struct finite_stop_callback_tuple
    : mcs::execution::finite_inplace_stop_callback<Count, Is, CB>...
{
    finite_stop_callback_tuple(mcs::execution::finite_inplace_stop_source<Count> &ss,
                               const CB &cb)
        : mcs::execution::finite_inplace_stop_callback<Count, Is, CB>{
              ss.template get_token<Is>(), cb}...
    {
    }
};

template <std::size_t Count, std::size_t CallbackCount>
void single_thread_register_multiple_with_stop_3()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        mcs::execution::finite_inplace_stop_source<Count> ss;
        std::size_t count = 0;
        auto cb = [&] {
            ++count;
        };
        using cb_t = decltype(cb);

        auto cbs = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return finite_stop_callback_tuple<Count, cb_t, Is...>{ss, cb};
        }(std::make_index_sequence<CallbackCount>{});

        ss.request_stop();

        if (count != CallbackCount)
        {
            std::terminate();
        }
    }
}

//
// Two-Threads Register/Unregister Callback
//

template <typename Func1, typename Func2>
std::vector<std::chrono::nanoseconds> run_two_threads_concurrently(Func1 func1,
                                                                   Func2 func2)
{

    std::atomic<bool> t1_ready{false};
    std::atomic<bool> t2_ready{false};

    auto compute_times = [&](auto &func, std::atomic<bool> &ready) {
        std::vector<std::chrono::nanoseconds> times;
        times.reserve(pass_count);

        for (std::uint32_t pass = 0; pass < pass_count; ++pass)
        {
            ready.store(true);
            while (ready.load())
            {
            }
            auto start = std::chrono::steady_clock::now();

            func();

            auto end = std::chrono::steady_clock::now();
            auto time = (end - start);
            times.push_back(time);
        }
        return times;
    };

    std::vector<std::chrono::nanoseconds> t1_times;
    std::vector<std::chrono::nanoseconds> t2_times;

    std::thread t1{[&] {
        t1_times = compute_times(func1, t1_ready);
    }};
    std::thread t2{[&] {
        t2_times = compute_times(func2, t2_ready);
    }};

    for (std::uint32_t pass = 0; pass < pass_count; ++pass)
    {
        while (!t1_ready.load())
        {
        }
        while (!t2_ready.load())
        {
        }
        t1_ready.store(false);
        t2_ready.store(false);
    }

    t1.join();
    t2.join();

    std::vector<std::chrono::nanoseconds> all_times;
    all_times.reserve(t1_times.size() + t2_times.size());
    all_times.insert(all_times.end(), t1_times.begin(), t1_times.end());
    all_times.insert(all_times.end(), t2_times.begin(), t2_times.end());
    return all_times;
}

void two_threads_register_unregister_1()
{
    mcs::execution::inplace_stop_source ss;

    auto register_callbacks = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::inplace_stop_callback scb{ss.get_token(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks, register_callbacks);

    print_times("  inplace_stop_source                              : ", times);
}

void two_threads_register_unregister_2()
{
    mcs::execution::single_inplace_stop_source ss1;
    mcs::execution::single_inplace_stop_source ss2;

    auto register_callbacks_1 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::single_inplace_stop_callback scb{ss1.get_token(), cb};
        }
    };

    auto register_callbacks_2 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::single_inplace_stop_callback scb{ss2.get_token(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks_1, register_callbacks_2);

    print_times("  2x single_inplace_stop_source                    : ", times);
}

void two_threads_register_unregister_2a()
{
    alignas(64) mcs::execution::single_inplace_stop_source ss1;
    alignas(64) mcs::execution::single_inplace_stop_source ss2;

    auto register_callbacks_1 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::single_inplace_stop_callback scb{ss1.get_token(), cb};
        }
    };

    auto register_callbacks_2 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::single_inplace_stop_callback scb{ss2.get_token(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks_1, register_callbacks_2);

    print_times("  2x single_inplace_stop_source (no false sharing) : ", times);
}

void two_threads_register_unregister_3()
{
    mcs::execution::finite_inplace_stop_source<2> ss;

    auto register_callbacks_1 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::finite_inplace_stop_callback scb{ss.get_token<0>(), cb};
        }
    };

    auto register_callbacks_2 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            mcs::execution::finite_inplace_stop_callback scb{ss.get_token<1>(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks_1, register_callbacks_2);

    print_times("  finite_inplace_stop_source<2>                    : ", times);
}

int main()
{

    // std::print("Register/unregister stop-callbacks single-threaded (100k times)\n");
    std::cout << "Register/unregister stop-callbacks single-threaded (100k times)\n";
    {
        mcs::execution::inplace_stop_source ss;
        timed_invoke_multi("  inplace_stop_source        : ", pass_count,
                           [&] { single_thread_register_unregister_1(); });
    }

    {
        mcs::execution::single_inplace_stop_source ss;
        timed_invoke_multi("  single_inplace_stop_source : ", pass_count,
                           [&] { single_thread_register_unregister_2(); });
    }

    // std::print("\nCall request_stop() with no callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with no callbacks (100k times)\n";

    timed_invoke_multi("  inplace_stop_source            : ", pass_count,
                       [&] { single_thread_no_callback_stop_1(); });

    timed_invoke_multi("  1x single_inplace_stop_source  : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<1>(); });

    timed_invoke_multi("  2x single_inplace_stop_source  : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<2>(); });

    timed_invoke_multi("  finite_inplace_stop_source<2>  : ", pass_count,
                       [&] { single_thread_no_callback_stop_3<2>(); });

    timed_invoke_multi("  3x single_inplace_stop_source  : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<3>(); });

    timed_invoke_multi("  finite_inplace_stop_source<3>  : ", pass_count,
                       [&] { single_thread_no_callback_stop_3<3>(); });

    timed_invoke_multi("  10x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<10>(); });

    timed_invoke_multi("  finite_inplace_stop_source<10> : ", pass_count,
                       [&] { single_thread_no_callback_stop_3<10>(); });

    // std::print("\nCall request_stop() with 1/1 callback (100k times)\n");
    std::cout << "\nCall request_stop() with 1/1 callback (100k times)\n";
    timed_invoke_multi("  inplace_stop_source           : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<1>(); });

    timed_invoke_multi("  single_inplace_stop_source    : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<1, 1>(); });

    // std::print("\nCall request_stop() with 1/2 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 1/2 callbacks (100k times)\n";
    timed_invoke_multi("  2x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<2, 1>(); });

    timed_invoke_multi("  finite_inplace_stop_source<2> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<2, 1>(); });

    // std::print("\nCall request_stop() with 1/3 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 1/3 callbacks (100k times)\n";
    timed_invoke_multi("  3x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<3, 1>(); });

    timed_invoke_multi("  finite_inplace_stop_source<3> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<3, 1>(); });

    // std::print("\nCall request_stop() with 2/2 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 2/2 callbacks (100k times)\n";
    timed_invoke_multi("  inplace_stop_source           : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<2>(); });

    timed_invoke_multi("  2x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<2, 2>(); });

    timed_invoke_multi("  finite_inplace_stop_source<2> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<2, 2>(); });

    // std::print("\nCall request_stop() with 3/3 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 3/3 callbacks (100k times)\n";
    timed_invoke_multi("  inplace_stop_source           : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<3>(); });

    timed_invoke_multi("  3x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<3, 3>(); });

    timed_invoke_multi("  finite_inplace_stop_source<3> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<3, 3>(); });

    // std::print("\nCall request_stop() with 10/10 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 10/10 callbacks (100k times)\n";
    timed_invoke_multi("  inplace_stop_source            : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<10>(); });

    timed_invoke_multi("  10x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<10, 10>(); });

    timed_invoke_multi("  finite_inplace_stop_source<10> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<10, 10>(); });

    // std::print("\nRegister/unregister callbacks from two threads concurrently\n");
    std::cout << "\nRegister/unregister callbacks from two threads concurrently\n";
    two_threads_register_unregister_1();
    two_threads_register_unregister_2();
    two_threads_register_unregister_2a();
    two_threads_register_unregister_3();

    // Display information about sizes
    {
        std::cout << "\n"
                  << "Data-Structure Sizes\n"
                  << "--------------------\n";
        std::cout << "inplace_stop_source                 : "
                  << sizeof(mcs::execution::inplace_stop_source) << " bytes\n";

        std::cout << "single_inplace_stop_source          : "
                  << sizeof(mcs::execution::single_inplace_stop_source) << " bytes\n";

        std::cout << "finite_inplace_stop_source<2>       : "
                  << sizeof(mcs::execution::finite_inplace_stop_source<2>) << " bytes\n";

        std::cout << "finite_inplace_stop_source<3>       : "
                  << sizeof(mcs::execution::finite_inplace_stop_source<3>)
                  << " bytes\n\n";

        int x = 0;
        auto cb = [&x] {
            ++x;
        };

        // std::print("inplace_stop_callback               : {} bytes\n",
        //            sizeof(mcs::execution::inplace_stop_callback<decltype(cb)>));
        std::cout << "inplace_stop_callback               : "
                  << sizeof(mcs::execution::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout << "single_inplace_stop_callback               : "
                  << sizeof(mcs::execution::single_inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout << "finite_inplace_stop_callback               : "
                  << sizeof(
                         mcs::execution::finite_inplace_stop_callback<2, 0, decltype(cb)>)
                  << " bytes\n ";
        std::cout << "inplace_stop_source + 1x callbacks               : "
                  << sizeof(mcs::execution::inplace_stop_source) +
                         sizeof(mcs::execution::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout << "inplace_stop_source + 2x callbacks               : "
                  << sizeof(mcs::execution::inplace_stop_source) +
                         2 * sizeof(mcs::execution::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout
            << "finite_inplace_stop_source<2> + 2x callbacks               : "
            << sizeof(mcs::execution::finite_inplace_stop_source<2>) +
                   sizeof(
                       mcs::execution::finite_inplace_stop_callback<2, 0, decltype(cb)>) +
                   sizeof(
                       mcs::execution::finite_inplace_stop_callback<2, 1, decltype(cb)>)
            << " bytes\n ";

        // 输出第一组数据
        std::cout << "inplace_stop_source + 3x callbacks           : "
                  << sizeof(mcs::execution::inplace_stop_source) +
                         3 * sizeof(mcs::execution::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n";

        std::cout
            << "3x single_inplace_stop_source + 3x callbacks : "
            << 3 * sizeof(mcs::execution::single_inplace_stop_source) +
                   3 * sizeof(mcs::execution::single_inplace_stop_callback<decltype(cb)>)
            << " bytes\n";

        std::cout
            << "finite_inplace_stop_source<3> + 3x callbacks : "
            << sizeof(mcs::execution::finite_inplace_stop_source<3>) +
                   sizeof(
                       mcs::execution::finite_inplace_stop_callback<3, 0, decltype(cb)>) +
                   sizeof(
                       mcs::execution::finite_inplace_stop_callback<3, 1, decltype(cb)>) +
                   sizeof(
                       mcs::execution::finite_inplace_stop_callback<3, 2, decltype(cb)>)
            << " bytes\n\n";

        // 输出第二组数据
        std::cout << "inplace_stop_source + 10x callbacks            : "
                  << sizeof(mcs::execution::inplace_stop_source) +
                         10 * sizeof(mcs::execution::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n";

        std::cout
            << "10x single_inplace_stop_source + 10x callbacks : "
            << 10 * sizeof(mcs::execution::single_inplace_stop_source) +
                   10 * sizeof(mcs::execution::single_inplace_stop_callback<decltype(cb)>)
            << " bytes\n";

        std::cout
            << "finite_inplace_stop_source<10> + 10x callbacks : "
            << sizeof(mcs::execution::finite_inplace_stop_source<10>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 0,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 1,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 2,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 3,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 4,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 5,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 6,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 7,
                                                                       decltype(cb)>) +
                   sizeof(mcs::execution::finite_inplace_stop_callback<10, 8,
                                                                       decltype(cb)>) +
                   sizeof(
                       mcs::execution::finite_inplace_stop_callback<10, 9, decltype(cb)>)
            << " bytes\n";
    }
}
// NOLINTEND