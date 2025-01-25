#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
// NOLINTBEGIN
void thread_func()
{
    int local_var = 0; // 局部变量
    for (int i = 0; i < 100000; ++i)
    {
        local_var++; // 修改局部变量
    }
    assert(local_var == 100000); // 断言局部变量的值
}

int main()
{
    const int num_threads = 10;
    std::vector<std::thread> threads; // 使用 std::thread 作为模板参数

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_func);
    }

    for (auto &t : threads)
    {
        t.join();
    }

    std::cout << "All threads completed. Local variables are thread-safe!" << std::endl;
    return 0;
}
// NOLINTEND