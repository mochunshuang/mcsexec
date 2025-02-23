#include <iostream>
#include <map>
#include <mutex>
#include <chrono>
// NOLINTBEGIN
#include <iostream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

const int NUM_OPERATIONS = 100000;

// 传统互斥锁方案的单线程测试
void test_mutex_lock() noexcept
{
    std::mutex mutex;
    int counter = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++counter;
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Mutex lock: " << std::chrono::duration<double>(end - start).count()
              << " seconds\n";
}

// 自旋锁（基于CAS的无锁实现）
class SpinLock
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

  public:
    void lock() noexcept
    {
        while (flag.test_and_set(std::memory_order_acquire))
        {
            // 自旋等待
        }
    }
    void unlock() noexcept
    {
        flag.clear(std::memory_order_release);
    }
};

// 自旋锁单线程测试
void test_spin_lock() noexcept
{
    SpinLock spinlock;
    int counter = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        spinlock.lock();
        ++counter;
        spinlock.unlock();
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Spin lock:  " << std::chrono::duration<double>(end - start).count()
              << " seconds\n";
}

// 纯CAS无锁方案的单线程测试
void test_pure_cas() noexcept
{
    std::atomic<bool> cas_lock(false);
    int counter = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        bool expected = false;
        while (!cas_lock.compare_exchange_weak(expected, true, std::memory_order_acquire,
                                               std::memory_order_relaxed))
        {
            expected = false;
        }
        ++counter;
        cas_lock.store(false, std::memory_order_release);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Pure CAS:   " << std::chrono::duration<double>(end - start).count()
              << " seconds\n";
}

// 无锁单线程测试
void test_without_lock() noexcept
{
    int counter = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        ++counter;
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Without lock: " << std::chrono::duration<double>(end - start).count()
              << " seconds\n";
}

// 多线程测试函数
template <typename Lock, typename CriticalSection>
double run_multithread_test(const std::string &test_name, int num_threads, Lock &lock,
                            CriticalSection crit_sec) noexcept
{
    int counter = 0;
    const int ops_per_thread = NUM_OPERATIONS / num_threads;

    auto worker = [&]() {
        for (int i = 0; i < ops_per_thread; ++i)
        {
            lock.lock();
            crit_sec(counter);
            lock.unlock();
        }
    };

    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(worker);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto res = std::chrono::duration<double>(end - start).count();
    std::cout << test_name << " (" << num_threads << " threads): " << res << " seconds\n";
    return res;
}

// 传统互斥锁多线程测试
auto test_mutex_multithread(int num_threads) noexcept
{
    std::mutex mutex;
    return run_multithread_test("Mutex lock", num_threads, mutex,
                                [](int &counter) { ++counter; });
}

// 自旋锁多线程测试
auto test_spinlock_multithread(int num_threads) noexcept
{
    SpinLock spinlock;
    return run_multithread_test("Spin lock", num_threads, spinlock,
                                [](int &counter) { ++counter; });
}

// 纯CAS多线程测试
auto test_pure_cas_multithread(int num_threads) noexcept
{
    struct CASLock
    {
        std::atomic<bool> flag{false};

        void lock() noexcept
        {
            bool expected = false;
            while (!flag.compare_exchange_weak(expected, true, std::memory_order_acquire,
                                               std::memory_order_relaxed))
            {
                expected = false;
            }
        }
        void unlock() noexcept
        {
            flag.store(false, std::memory_order_release);
        }
    } casLock;

    return run_multithread_test("Pure CAS", num_threads, casLock,
                                [](int &counter) { ++counter; });
}
void print_comparison(double mutex, double spin, double cas) noexcept
{
    // 将结果存储到map中，方便排序
    std::map<double, std::string> results;
    results[mutex] = "mutex";
    results[spin] = "spin";
    results[cas] = "cas";

    // 将结果按时间从小到大排序
    std::vector<std::pair<double, std::string>> sorted_results(results.begin(),
                                                               results.end());

    // 输出结论
    std::cout << "time cost: ";
    for (size_t i = 0; i < sorted_results.size(); ++i)
    {
        std::cout << sorted_results[i].second;
        if (i < sorted_results.size() - 1)
        {
            std::cout << " < ";
        }
    }
    std::cout << std::endl;
}
// NOTE: std::lock_guard<std::mutex> is best good
int main()
{
    // 单线程测试
    std::cout << "Single thread tests:\n";
    test_without_lock();
    test_mutex_lock();
    test_spin_lock();
    test_pure_cas();
    test_spin_lock(); // 重复测试以观察一致性

    // 多线程测试（2-9个线程）
    for (int num_threads = 2; num_threads <= 9; ++num_threads)
    {
        std::cout << "\nTesting with " << num_threads << " threads:\n";
        double mutex = test_mutex_multithread(num_threads);
        double spin = test_spinlock_multithread(num_threads);
        double cas = test_pure_cas_multithread(num_threads);
        // 设计一个算法，高可读输出结论，比如 time cost: cas < spin < mutex
        print_comparison(mutex, spin, cas);
    }

    return 0;
}

// NOLINTEND