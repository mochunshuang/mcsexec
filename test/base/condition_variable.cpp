#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>

int main()
{
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = true;

    std::unique_lock<std::mutex> lock(mtx); // 主线程获取锁
    std::cout << "Main thread has acquired the lock." << '\n';
    std::cout << "Main thread is waiting..." << '\n';
    // Note: wait 的时候,会先检查一遍.第二次检查,由 notify_* 来确定
    cv.wait(lock, [&] { return ready; });
    std::cout << "Main thread is running again!" << '\n';
    lock.unlock(); // Note: 不释放,再次获取就是死锁
    {
        ready = false;
        std::jthread t{[&]() {
            std::cout << "hello 0\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ready = true;
            cv.notify_one(); // 没有这一句会死锁
        }};
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return ready; });
        std::cout << "Main: 0" << '\n';
    }
    {
        bool ready = false;
        std::jthread t{[&]() {
            std::cout << "hello 1\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ready = true;
            cv.notify_one();
        }};
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return ready; });
        std::cout << "Main: 1" << '\n';
    }
    {
        ready = false;
        std::jthread t{[&]() {
            std::cout << "hello 2\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ready = true;
            std::unique_lock<std::mutex> lock(mtx); // 没有死锁
            cv.notify_one();
        }};
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return ready; });
        std::cout << "Main: 2" << '\n';
    }
    {
        ready = false;
        std::jthread t{[&]() {
            std::cout << "hello 3\n";
            // 一样不造成死锁
            std::unique_lock<std::mutex> lock(mtx);
            ready = true;
            cv.notify_one();
        }};
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return ready; }); // 会释放锁
        std::cout << "Main: 3" << '\n';
    }

    {
        ready = false;
        std::jthread t{[&]() {
            std::cout << "hello 4\n";
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟耗时操作
            std::unique_lock<std::mutex> lock(mtx);               // 子线程获取锁
            ready = true;
            // Note: wait 和  notify_ 必须一起使用
            // 故意不调用 cv.notify_one()，模拟虚假唤醒
            cv.notify_one();
        }};

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return ready; });
        std::cout << "Main: 4" << '\n'; // 如果虚假唤醒发生，这里会输出
    }
    {
        for (int i = 0; i < 1000; ++i) // NOLINT
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return i >= 0; });
            if (i % 100 == 0) // NOLINT
                std::cout << "Main: for:" << i << '\n';
        }
    }

    return 0;
}