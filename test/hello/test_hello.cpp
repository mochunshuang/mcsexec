#include <cassert>
#include <iostream>

int add(int a, int b)
{
    return a + b;
};

int main()
{
    assert(add(1, 2) == 3);
    // assert(1 == 3); // 每捕获抛异常强制
    std::cout << "test done!\n";
    return 0;
}