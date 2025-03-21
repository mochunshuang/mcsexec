#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

// NOLINTBEGIN
struct cas_lock
{
    std::atomic<bool> lock_ = {false};

    void lock() noexcept
    {
        bool expected = false;
        while (!lock_.compare_exchange_weak(expected, true, std::memory_order_acquire))
        {
            expected = false;
            std::this_thread::yield(); // 避免忙等待
        }
    }

    void unlock() noexcept
    {
        lock_.store(false, std::memory_order_release);
    }
};
struct spin_lock
{
    mutable std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

    void lock() const noexcept
    {
        while (lock_.test_and_set(std::memory_order_acquire))
        {
            lock_.wait(true, std::memory_order_relaxed);
        }
    }

    bool try_lock() const noexcept
    {
        return !lock_.test_and_set(std::memory_order_acquire);
    }

    void unlock() const noexcept
    {
        lock_.clear(std::memory_order_release);
        lock_.notify_one();
    }
};

#include <mutex>

struct mutex_lock
{
    std::mutex mtx_;

    void lock() noexcept
    {
        mtx_.lock();
    }

    void unlock() noexcept
    {
        mtx_.unlock();
    }
};

template <typename Lock>
void benchmark_lock(Lock &lock, int num_threads, int num_iterations)
{
    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&lock, num_iterations]() {
            for (int j = 0; j < num_iterations; ++j)
            {
                lock.lock();
                // 模拟一些工作
                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
                lock.unlock();
            }
        });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << "s\n";
}

int main()
{
    // NOTE: Testing CAS lock: 3 个线程以下是最好的
    const int num_threads = 2;
    const int num_iterations = 100000;

    cas_lock cas;
    spin_lock spin;
    mutex_lock mtx;

    std::cout << "Testing CAS lock:\n";
    benchmark_lock(cas, num_threads, num_iterations);

    std::cout << "Testing Spin lock:\n";
    benchmark_lock(spin, num_threads, num_iterations);

    std::cout << "Testing std::mutex:\n";
    benchmark_lock(mtx, num_threads, num_iterations);

    return 0;
}
// NOLINTEND