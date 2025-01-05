#include <iostream>
#include <type_traits>
#include <utility>

// 用于测试的类
struct A
{
};

// 测试函数模板
template <typename T>
void test_forward(T &&arg) // NOLINT
{
    auto &&result = std::forward<decltype((arg))>(arg);
    static_assert(std::is_lvalue_reference_v<decltype(result)>);
}

int main()
{
    // 测试右值（例如 1 或 A{}）
    std::cout << "Testing rvalue (1):\n";
    test_forward(1);

    std::cout << "\nTesting rvalue (A{}):\n";
    test_forward(A{});

    // 测试左值（例如 int x 或 A a）
    int x = 42;
    std::cout << "\nTesting lvalue (int x):\n";
    test_forward(x);

    A a;
    std::cout << "\nTesting lvalue (A a):\n";
    test_forward(a);

    return 0;
}