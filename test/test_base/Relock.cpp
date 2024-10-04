#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>
#include <memory>

// 定义一个简单的线程安全队列
template <typename T>
class ThreadSafeQueue
{
  public:
    void push(T value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
    }

    bool pop(T &value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty())
        {
            return false;
        }
        value = queue_.front();
        queue_.pop();
        return true;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::mutex &getMutex()
    {
        return mutex_;
    }

  private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
};

// 定义一个 Relock 类，用于临时释放锁并在析构时重新获取锁
class Relock
{
  public:
    explicit Relock(std::unique_lock<std::mutex> &lock) : lock_(lock)
    {
        lock_.unlock();
    }

    ~Relock()
    {
        lock_.lock();
    }

  private:
    std::unique_lock<std::mutex> &lock_;
};

// 处理队列中的元素的函数
void processQueue(ThreadSafeQueue<int> &queue)
{
    while (true)
    {
        int value;
        {
            std::unique_lock<std::mutex> lock(queue.getMutex());
            if (!queue.pop(value))
            {
                break;
            }
            // 使用 Relock 临时释放锁
            Relock relock(lock);
            // 模拟处理元素的耗时操作
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Processed value: " << value << std::endl;
        }
    }
}

int main()
{
    ThreadSafeQueue<int> queue;

    // 向队列中添加一些元素
    for (int i = 0; i < 10; ++i)
    {
        queue.push(i);
    }

    // 创建两个线程来处理队列中的元素
    std::thread t1(processQueue, std::ref(queue));
    std::thread t2(processQueue, std::ref(queue));

    t1.join();
    t2.join();

    return 0;
}