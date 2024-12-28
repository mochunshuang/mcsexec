#include <cassert>
#include <iostream>

int add(int a, int b)
{
    return a + b;
};

int main()
{
    assert(add(1, 2) == 3);
    std::cout << "test done!\n";
    return 0;
}