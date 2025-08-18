#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

class SpinLock
{
  public:
    std::atomic_flag lock_; // NOLINT

    void lock() noexcept
    {
        while (lock_.test_and_set(std::memory_order_acquire))
            // C++ 20 起可以仅在 unlock 中通知后才获得锁，从而避免任何无效自旋
            // 注意，即使 wait
            // 保证一定在值被更改后才返回，但锁定是在下一次执行条件时完成的
            lock_.wait(true, std::memory_order_relaxed);
    }
    bool try_lock() noexcept // NOLINT
    {
        return !lock_.test_and_set(std::memory_order_acquire);
    }
    void unlock() noexcept
    {
        lock_.clear(std::memory_order_release);
        lock_.notify_one();
    }
};

// 共享资源
std::vector<int *> shared_value;
SpinLock spin_lock;
std::mutex std_mutex;

void increment_with_spinlock(int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        spin_lock.lock();
        shared_value.push_back(new int);
        spin_lock.unlock();
    }
    for (int i = 0; i < iterations; ++i)
    {
        spin_lock.lock();
        // 删除
        delete shared_value.back(); // 释放内存
        shared_value.pop_back();
        spin_lock.unlock();
    }
}

void increment_with_mutex(int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(std_mutex);
        shared_value.push_back(new int);
    }
    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(std_mutex);
        // 删除
        delete shared_value.back(); // 释放内存
        shared_value.pop_back();
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
    // 3 个线程以上，std::mutex bettter。 长时间操作，还是std::mutex bettter
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
        // 6 个线程以下，2个以上，差不多。 同一个数量级的
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