// NOLINTBEGIN
#include <iostream>

struct universal_arg
{
    // 支持隐式转换到任意类型（包括引用）
    template <typename T>
    consteval operator T &&() const noexcept; // 无需定义，仅用于类型推导
};

template <typename F>
concept ZeroArg = requires(F f) { f(); };

template <typename F>
concept OneArg = requires(F f) { f(universal_arg{}); };

template <typename F>
concept TwoArg = requires(F f) { f(universal_arg{}, universal_arg{}); };

template <typename F>
struct arg_count
{
    static constexpr size_t value = [] {
        if constexpr (ZeroArg<F>)
            return 0;
        else if constexpr (OneArg<F>)
            return 1;
        else if constexpr (TwoArg<F>)
            return 2;
        else
            static_assert(false, "Unsupported argument count");
    }();
};

// 测试函数
void func1(int) {}
void func2(int, double) {}
auto lambda0 = [] {
};
auto lambda1 = [](auto x) {
};
auto lambda2 = [](auto a, auto b) {
};

int main()
{
    std::cout << arg_count<decltype(func1)>::value << '\n';   // 输出1
    std::cout << arg_count<decltype(func2)>::value << '\n';   // 输出2
    std::cout << arg_count<decltype(lambda0)>::value << '\n'; // 输出0（需特化支持）
    std::cout << arg_count<decltype(lambda1)>::value << '\n'; // 输出1
    std::cout << arg_count<decltype(lambda2)>::value << '\n'; // 输出2
}
// NOLINTEND