#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

class MutexChecker
{
  private:
    std::mutex mtx; // 内部管理的互斥锁 // NOLINT

  public:
    /**
     * @brief
     * Note: 确定，要获得锁。 原子变量记录状态最好。 std::shared_mutex 多读少写才有效果
     *
     * @return true
     * @return false
     */
    // 检查 mtx 是否被占用的函数
    bool is_locked() // NOLINT
    {
        if (mtx.try_lock())
        {
            mtx.unlock(); // 如果成功获取锁，立即释放
            return false; // 锁未被占用
        }
        return true; // 锁被占用
    }

    // 模拟占用锁的函数
    void lock_and_sleep(int milliseconds) // NOLINT
    {
        std::lock_guard<std::mutex> lk(mtx);
        std::cout << "Thread 1: Lock acquired, sleeping for " << milliseconds << "ms"
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        std::cout << "Thread 1: Lock released" << std::endl;
    }
};

// 线程 1：模拟占用锁
void thread1_func(MutexChecker &checker)
{
    int count = 2;
    while ((count--) != 0)
    {
        checker.lock_and_sleep(150); // 模拟占用锁  // NOLINT
        std::this_thread::sleep_for(std::chrono::milliseconds(20)); // NOLINT
    }
}

// 线程 2：检查锁是否被占用
void thread2_func(MutexChecker &checker)
{
    int count = 10; // NOLINT
    while ((count--) != 0)
    {
        bool locked = checker.is_locked();
        std::cout << "Thread 2: mtx is " << (locked ? "locked" : "unlocked") << std::endl;
        std::this_thread::sleep_for(
            std::chrono::milliseconds(20)); // 模拟检查频率 // NOLINT
    }
}

int main()
{
    MutexChecker checker;

    std::thread t1(thread1_func, std::ref(checker));
    std::thread t2(thread2_func, std::ref(checker));

    t1.join();
    t2.join();

    return 0;
}