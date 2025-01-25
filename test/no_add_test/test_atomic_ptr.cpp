#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
// NOLINTBEGIN
struct Data
{
    int value; // 非原子变量
};

std::atomic<Data *> ptr{nullptr};

void thread_func()
{
    for (int i = 0; i < 100000; ++i)
    {
        Data *local_ptr = ptr.load(std::memory_order_relaxed);
        if (local_ptr)
        {
            local_ptr->value++; // 非原子修改
        }
    }
}

int main()
{
    Data *data = new Data{0};
    ptr.store(data, std::memory_order_relaxed);

    const int num_threads = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_func);
    }

    for (auto &t : threads)
    {
        t.join();
    }
    const int expected_value = num_threads * 100000; // 预期值
    std::cout << "Expected value: " << expected_value << std::endl;
    std::cout << "Final value: " << data->value << std::endl;
    assert(expected_value != data->value);

    delete data;

    return 0;
}
// NOLINTEND