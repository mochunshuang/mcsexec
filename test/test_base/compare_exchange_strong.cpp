#include <iostream>
#include <stdatomic.h>

int main()
{
    std::atomic<int> atomic_int(10);
    int expected = 10;
    int desired = 20;

    /**
     * @brief  compare_exchange_strong 的第一个参数 是 引用，出参
     *
     */
    // 第一次尝试更新
    if (atomic_int.compare_exchange_strong(expected, desired))
    {
        std::cout << "Success: expected = " << expected
                  << ", atomic_int = " << atomic_int.load() << std::endl;
    }
    else
    {
        std::cout << "Failure: expected = " << expected
                  << ", atomic_int = " << atomic_int.load() << std::endl;
    }

    // 第二次尝试更新
    expected = 15; // 假设原子变量的值被其他线程修改为 15
    desired = 30;

    if (atomic_int.compare_exchange_strong(expected, desired))
    {
        std::cout << "Success: expected = " << expected
                  << ", atomic_int = " << atomic_int.load() << std::endl;
    }
    else
    {
        std::cout << "Failure: expected = " << expected
                  << ", atomic_int = " << atomic_int.load() << std::endl;
    }

    return 0;
}