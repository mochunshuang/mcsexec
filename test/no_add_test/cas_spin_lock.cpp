// NOLINTBEGIN
#include <atomic>
#include <cassert>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

struct spin_lock
{
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

    void lock() noexcept
    {
        while (lock_.test_and_set(std::memory_order_acquire))
        {
            lock_.wait(true, std::memory_order_relaxed);
        }
    }

    bool try_lock() noexcept
    {
        return !lock_.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        lock_.clear(std::memory_order_release);
        lock_.notify_one();
    }
};

struct cas_spin_lock
{
    std::atomic<bool> locked_ = false;

    void lock() noexcept
    {
        bool expected = false;
        while (!locked_.compare_exchange_weak(expected, true, std::memory_order_release,
                                              std::memory_order_relaxed))
        {
            expected = false; // 重置期望值
        }
    }

    bool try_lock() noexcept
    {
        bool expected = false;
        return locked_.compare_exchange_weak(expected, true, std::memory_order_release,
                                             std::memory_order_relaxed);
    }

    void unlock() noexcept
    {
        locked_.store(false, std::memory_order_release);
        locked_.notify_one();
    }
};

void warm_up()
{
    std::atomic<uint16_t> counter = 0;

    // 单线程执行简单任务，预热CPU缓存和内存
    for (int j = 0; j < std::numeric_limits<uint16_t>::max(); ++j)
    {
        ++counter;
    }
}

template <typename Lock>
void test_lock(Lock &lock, int num_threads, int iterations)
{
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < iterations; ++j)
            {
                lock.lock();
                ++counter;
                lock.unlock();
            }
        });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Time taken: " << duration << " ns" << std::endl;
    std::cout << "Counter: " << counter.load()
              << " (Expected: " << num_threads * iterations << ")\n"
              << std::endl;
}

void test_lock_guard(int num_threads, int iterations)
{
    std::mutex mutex;
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < iterations; ++j)
            {
                std::lock_guard lock(mutex);
                ++counter;
            }
        });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Time taken: " << duration << " ns" << std::endl;
    std::cout << "Counter: " << counter.load()
              << " (Expected: " << num_threads * iterations << ")\n"
              << std::endl;
}

int main()
{
    constexpr int num_threads = 2;        // 线程数
    constexpr int iterations = 1'000'000; // 每个线程的迭代次数

    warm_up();
    // 测试 atomic_flag 版本
    {
        spin_lock lock;
        std::cout << "Testing atomic_flag version:" << std::endl;
        test_lock(lock, num_threads, iterations);
    }
    warm_up();
    // 测试 CAS 版本
    {
        cas_spin_lock lock;
        std::cout << "Testing CAS version:" << std::endl;
        test_lock(lock, num_threads, iterations);
    }
    warm_up();
    {
        std::cout << "Testing lock_guard version:" << std::endl;
        test_lock_guard(num_threads, iterations);
    }

    {

        auto num_threads = 8;
        auto iterations = 100;
        std::cout << "num_threads: " << num_threads << " , iterations: " << iterations
                  << '\n';
        // 测试 atomic_flag 版本
        {
            warm_up();
            spin_lock lock;
            std::cout << "Testing atomic_flag version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        // 测试 CAS 版本
        {
            warm_up();
            cas_spin_lock lock;
            std::cout << "Testing CAS version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }
        {
            warm_up();
            std::cout << "Testing lock_guard version:" << std::endl;
            test_lock_guard(num_threads, iterations);
        }
    }
    {

        auto num_threads = 2;
        auto iterations = 100;
        std::cout << "num_threads: " << num_threads << " , iterations: " << iterations
                  << '\n';
        // 测试 atomic_flag 版本
        {
            warm_up();
            spin_lock lock;
            std::cout << "Testing atomic_flag version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        // 测试 CAS 版本
        {
            warm_up();
            cas_spin_lock lock;
            std::cout << "Testing CAS version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        {
            warm_up();
            std::cout << "Testing lock_guard version:" << std::endl;
            test_lock_guard(num_threads, iterations);
        }
    }
    {

        auto num_threads = 2;
        auto iterations = 10;
        std::cout << "num_threads: " << num_threads << " , iterations: " << iterations
                  << '\n';
        // 测试 atomic_flag 版本
        {
            warm_up();
            spin_lock lock;
            std::cout << "Testing atomic_flag version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        // 测试 CAS 版本
        {
            warm_up();
            cas_spin_lock lock;
            std::cout << "Testing CAS version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        {
            warm_up();
            std::cout << "Testing lock_guard version:" << std::endl;
            test_lock_guard(num_threads, iterations);
        }
    }
    // NOTE: 很低的竞争下,没有多少区别. 较低的竞争
    {

        auto num_threads = 1;
        auto iterations = 1;
        std::cout << "num_threads: " << num_threads << " , iterations: " << iterations
                  << '\n';
        // 测试 atomic_flag 版本
        {
            warm_up();
            spin_lock lock;
            std::cout << "Testing atomic_flag version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        // 测试 CAS 版本
        {
            warm_up();
            cas_spin_lock lock;
            std::cout << "Testing CAS version:" << std::endl;
            test_lock(lock, num_threads, iterations);
        }

        {
            warm_up();
            std::cout << "Testing lock_guard version:" << std::endl;
            test_lock_guard(num_threads, iterations);
        }
    }
    // NOTE: 短代码块,且,竞争低的情况下. lock_guard比较稳定 是不错的
    // NOTE: mutex 提供排他性非递归所有权语义：这是缺陷
    // NOTE: 不考虑 mutex 的情况下. 1 两个线程竞争,CAS 更好
    {
        constexpr auto count = 100;
        std::cout << "循环" << count << "次比较" << '\n';

        {
            warm_up();
            spin_lock lock;
            auto start = std::chrono::high_resolution_clock::now();
            lock.lock();
            int v{};
            for (int i = 0; i < count; i++) // NOLINT
            {
                v++;
            }
            lock.unlock();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            std::cout << "spin_lock : Time taken: " << duration << " ns\nn";
        }
        {
            warm_up();
            cas_spin_lock lock;
            auto start = std::chrono::high_resolution_clock::now();
            lock.lock();
            int v{};
            for (int i = 0; i < count; i++) // NOLINT
            {
                v++;
            }
            lock.unlock();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            std::cout << "cas_spin_lock : Time taken: " << duration << " ns\nn";
        }
        {
            warm_up();

            std::mutex mutex;

            auto start = std::chrono::high_resolution_clock::now();

            {
                std::lock_guard lock(mutex);
                int v{};
                for (int i = 0; i < count; i++) // NOLINT
                {
                    v++;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            std::cout << "lock_guard : Time taken: " << duration << " ns\nn";
        }
        //--
        int a = 1;
        if (1 == (a--))
        {
        }
        else
        {
            assert(false);
        }
    }

    return 0;
}

// NOLINTEND