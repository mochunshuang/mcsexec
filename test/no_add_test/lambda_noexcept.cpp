#include <iostream>

int main()
{
    // lambda 可以像普通函数一样
    {
        auto fun = [](auto &&...) {
        };
        static_assert(not noexcept(fun(1, 2)));
    }
    {
        auto fun = [](auto &&...) noexcept(true) {
        };
        static_assert(noexcept(fun(1, 2)));
    }
    {
        auto fun = [](auto &&...) noexcept(false) {
        };
        static_assert(not noexcept(fun(3)));
    }
    std::cout << "main done\n";
    return 0;
}