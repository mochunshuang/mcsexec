#include <atomic>
#include <thread>
#include <iostream>

class SpinLock
{
  public:
    SpinLock() noexcept : m_locked(false) {}

    void lock() noexcept
    {
        bool expected = false;
        while (!m_thread_holding_lock && !m_locked.compare_exchange_weak(expected, true))
        {
            expected = false;          // 重置 expected
            std::this_thread::yield(); // 让出 CPU
        }
        m_thread_holding_lock = true; // 标记当前线程持有锁
    }

    void unlock() noexcept
    {
        if (m_thread_holding_lock)
        {
            m_thread_holding_lock = false;                    // 清除当前线程的持有标记
            m_locked.store(false, std::memory_order_release); // 释放锁
        }
    }

  private:
    std::atomic<bool> m_locked; // 锁状态
    thread_local static bool m_thread_holding_lock;
};
thread_local bool SpinLock::m_thread_holding_lock = false;

// 测试代码
int main()
{
    SpinLock spinLock;

    auto task = [&spinLock]() {
        spinLock.lock();
        spinLock.lock();
        spinLock.lock();
        std::cout << "Thread " << std::this_thread::get_id() << " acquired the lock.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟工作
        spinLock.unlock();
        std::cout << "Thread " << std::this_thread::get_id() << " released the lock.\n";
    };

    std::thread t1(task);
    std::thread t2(task);

    t1.join();
    t2.join();

    return 0;
}