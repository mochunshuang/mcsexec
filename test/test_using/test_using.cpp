
#include <iostream>

// 使用类型，还是使用实例。要想清楚
#include "./receiver.hpp"

int main()
{
    // ODR不违背的情况下，如何满足上述要求
    mcs::execution::receiver.recr_hello();
    std::cout << "ptr: " << &mcs::execution::receiver << '\n';
    return 0;
}