#include <atomic>
#include <iostream>

int main()
{
    std::atomic<int> value(10);
    int expected = 10;
    int desired = 20;

    // Note: 如果 value 是 expected的值，会返回 true
    if (value.compare_exchange_strong(expected, desired))
    {
        std::cout << "更新成功，value = " << value.load() << std::endl;
    }
    else
    {
        std::cout << "更新失败，expected = " << expected << std::endl;
    }

    return 0;
}