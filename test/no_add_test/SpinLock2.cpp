#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

class SpinLock
{
  public:
    SpinLock() noexcept : m_locked(false) {}

    void lock() noexcept
    {
        bool expected = false;
        while (!m_locked.compare_exchange_weak(expected, true))
        {
            expected = false;          // 重置 expected
            std::this_thread::yield(); // 让出 CPU
        }
    }

    void unlock() noexcept
    {
        m_locked.store(false, std::memory_order_release); // 释放锁
    }

  private:
    std::atomic<bool> m_locked; // 锁状态
};

// 共享资源
int shared_value = 0;
SpinLock spin_lock;
std::mutex std_mutex;

void increment_with_spinlock(int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        spin_lock.lock();
        ++shared_value;
        spin_lock.unlock();
    }
}

void increment_with_mutex(int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(std_mutex);
        ++shared_value;
    }
}

void test_performance(int num_threads)
{

    const int iterations_per_thread = 100000;

    // 测试 SpinLock
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> spinlock_threads;
    for (int i = 0; i < num_threads; ++i)
    {
        spinlock_threads.emplace_back(increment_with_spinlock, iterations_per_thread);
    }
    for (auto &t : spinlock_threads)
    {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> spinlock_duration = end - start;
    std::cout << "SpinLock time: " << spinlock_duration.count() << " seconds"
              << std::endl;

    // 测试 std::mutex
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> mutex_threads;
    for (int i = 0; i < num_threads; ++i)
    {
        mutex_threads.emplace_back(increment_with_mutex, iterations_per_thread);
    }
    for (auto &t : mutex_threads)
    {
        t.join();
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mutex_duration = end - start;
    std::cout << "std::mutex time: " << mutex_duration.count() << " seconds" << std::endl;
    if (spinlock_duration.count() < mutex_duration.count())
        std::cout << "num_threads: " << num_threads << " , spinlock bettter \n";
    else
        std::cout << "num_threads: " << num_threads << " , std::mutex bettter \n";
}

void test_performance2(int num_threads)
{

    const int iterations_per_thread = 100;

    // 测试 SpinLock
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> spinlock_threads;
    for (int i = 0; i < num_threads; ++i)
    {
        spinlock_threads.emplace_back(increment_with_spinlock, iterations_per_thread);
    }
    for (auto &t : spinlock_threads)
    {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> spinlock_duration = end - start;
    std::cout << "SpinLock time: " << spinlock_duration.count() << " seconds"
              << std::endl;

    // 测试 std::mutex
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> mutex_threads;
    for (int i = 0; i < num_threads; ++i)
    {
        mutex_threads.emplace_back(increment_with_mutex, iterations_per_thread);
    }
    for (auto &t : mutex_threads)
    {
        t.join();
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mutex_duration = end - start;
    std::cout << "std::mutex time: " << mutex_duration.count() << " seconds" << std::endl;
    if (spinlock_duration.count() < mutex_duration.count())
        std::cout << "num_threads: " << num_threads << " , spinlock bettter \n";
    else
        std::cout << "num_threads: " << num_threads << " , std::mutex bettter \n";
}

int main()
{
    // 毫无疑问，状态简单的情况下 std_mutex 更好。 频繁的加锁和释放锁
    test_performance(1);
    std::cout << '\n';
    test_performance(2);
    std::cout << '\n';
    test_performance(3);
    std::cout << '\n';
    test_performance(4);
    std::cout << '\n';
    test_performance(5); // NOLINT
    std::cout << '\n';
    test_performance(6); // NOLINT
    std::cout << '\n';
    test_performance(7); // NOLINT
    std::cout << '\n';
    test_performance(8); // NOLINT
    std::cout << '\n';
    test_performance(9); // NOLINT
    std::cout << '\n';
    test_performance(10); // NOLINT
    std::cout << '\n';
    std::cout << "\ntest_performance2 " << '\n';
    {
        // 简单的，仅仅是加锁和释放锁，还是 std::mutex 更好。
        test_performance2(1);
        std::cout << '\n';
        test_performance2(2);
        std::cout << '\n';
        test_performance2(3);
        std::cout << '\n';
        test_performance2(4);
        std::cout << '\n';
        test_performance2(5); // NOLINT
        std::cout << '\n';
        test_performance2(6); // NOLINT
        std::cout << '\n';
        test_performance2(7); // NOLINT
        std::cout << '\n';
        test_performance2(8); // NOLINT
        std::cout << '\n';
        test_performance2(9); // NOLINT
        std::cout << '\n';
        test_performance2(10); // NOLINT
        test_performance2(20); // NOLINT 例外，但是很少有那么多线程抢占
    }
    return 0;
}