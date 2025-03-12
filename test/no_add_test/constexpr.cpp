
#include <iostream>

// NOLINTBEGIN

constexpr int add(int a, int b)
{
    return a + b;
}
void test0()
{
    constexpr int x = 5, y = 10;
    constexpr int z = add(x, y); // ✅ 编译期计算
    {
        static_assert(add(1, 1) == 2);
        static_assert(z == 15);
    }
    // int a = 5, b = 10;
    // int c = add(a, b); // ❌ 运行时计算，但函数仍可调用
}
void test1()
{
    // 无捕获 Lambda：隐式 constexpr
    constexpr auto lambda = [](int x) {
        return x * 2;
    };
    static_assert(lambda(5) == 10); // ✅ C++17+

    // 捕获 constexpr 变量：隐式 constexpr
    constexpr int n = 5;
    constexpr auto lambda_cap = [n](int x) {
        return x + n;
    };
    static_assert(lambda_cap(5) == 10); // ✅ C++17+
}

// 定义管道操作符 | 的辅助类
struct PipeHelper
{
    template <typename F1, typename F2>
    constexpr static auto combine(F1 f1, F2 f2)
    {
        return [=](auto x) {
            return f2(f1(x));
        };
    }
};

// 定义 constexpr operator|
template <typename F1, typename F2>
constexpr auto operator|(F1 f1, F2 f2)
{
    return PipeHelper::combine(f1, f2);
}

void test2()
{
    {
        constexpr auto result = [](int x) {
            return x + 1;
        } | [](int x) {
            return x * 2;
        };

        static_assert(result(5) == 12); // ✅ 编译期计算
    }

    {
        constexpr auto f2 = [](int x) {
            return x * 2;
        };
        constexpr auto result = [](int x) {
            return x + 1;
        } | f2; // ✅ 纯右值 + constexpr

        static_assert(result(5) == 12);
    }
    {
        constexpr auto f1 = [](int x) {
            return x + 1;
        };
        constexpr auto result = f1 | [](int x) {
            return x * 2;
        }; // ✅ constexpr + 纯右值

        static_assert(result(5) == 12);
    }
    {
        constexpr auto f1 = [](int x) {
            return x + 1;
        };
        constexpr auto f2 = [](int x) {
            return x * 2;
        };
        constexpr auto result = f1 | f2; // ✅ constexpr + constexpr

        static_assert(result(5) == 12);
    }
}
void test2_1()
{
    int a = 5;
    auto f_runtime = [a](int x) {
        return x + a;
    }; // 捕获运行时变量
    // constexpr auto result = f_runtime | [](int x) {
    //     return x * 2;
    // }; // ❌ 无法编译期计算
}

int main()
{

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND