#include <algorithm>
#include <cassert>
#include <exception>
#include <type_traits>
#include <utility>
#include <iostream>
#include <vector>

// NOLINTBEGIN

// 自定义类型用于测试
struct MyStruct
{
    int value;
    void method() const {}
};

// 函数用于测试函数类型推导
void test_function(int) {}

template <typename T>
auto test_template_type(T &&type)
{
    using T0 = T;
    using T1 = decltype((type));
    using T2 = decltype(type); // NOTE: 正是 目标函数 传入的类型
    if (!std::is_same_v<T0, T2>)
        std::cout << ">>> T0 != T2\n"; // 打印一般都是: T != T&&

    if (std::is_lvalue_reference_v<decltype(type)>)
    {
        if (not std::is_same_v<T0, T2>)
            std::terminate();
    }

    using T3 = decltype((std::forward_like<T>(type)));
    using T4 = decltype((std::forward_like<decltype(type)>(type)));
    using T5 = decltype((std::forward_like<T>(std::forward<T>(type))));
    using T6 = decltype((std::forward_like<decltype(type)>(std::forward<T>(type))));

    using T7 = decltype((std::forward<T>(type)));
    using T8 = decltype((std::forward<decltype(type)>(type)));
    using T9 = decltype((std::forward<T>(std::forward<T>(type))));
    using T10 = decltype((std::forward<decltype(type)>(std::forward<T>(type))));

    struct result_types
    {
        using t0 = T0;
        using t1 = T1;
        using t2 = T2;
        using t3 = T3;
        using t4 = T4;
        using t5 = T5;
        using t6 = T6;
        using t7 = T7;
        using t8 = T8;
        using t9 = T9;
        using t10 = T10;
    };
    return result_types{};
}

// 类型名称打印工具
template <typename T>
constexpr const char *type_name()
{
    // 基本类型
    if constexpr (std::is_same_v<T, int>)
        return "int";
    if constexpr (std::is_same_v<T, const int>)
        return "const int";
    if constexpr (std::is_same_v<T, volatile int>)
        return "volatile int";
    if constexpr (std::is_same_v<T, const volatile int>)
        return "const volatile int";

    // 引用类型
    if constexpr (std::is_same_v<T, int &>)
        return "int&";
    if constexpr (std::is_same_v<T, const int &>)
        return "const int&";
    if constexpr (std::is_same_v<T, volatile int &>)
        return "volatile int&";
    if constexpr (std::is_same_v<T, const volatile int &>)
        return "const volatile int&";
    if constexpr (std::is_same_v<T, int &&>)
        return "int&&";
    if constexpr (std::is_same_v<T, const int &&>)
        return "const int&&";
    if constexpr (std::is_same_v<T, volatile int &&>)
        return "volatile int&&";
    if constexpr (std::is_same_v<T, const volatile int &&>)
        return "const volatile int&&";

    // 自定义类型
    if constexpr (std::is_same_v<T, MyStruct>)
        return "MyStruct";
    if constexpr (std::is_same_v<T, MyStruct &>)
        return "MyStruct&";
    if constexpr (std::is_same_v<T, const MyStruct &>)
        return "const MyStruct&";
    if constexpr (std::is_same_v<T, MyStruct &&>)
        return "MyStruct&&";

    // 函数类型
    if constexpr (std::is_same_v<T, void (*)(int)>)
        return "void(*)(int)";
    if constexpr (std::is_same_v<T, void (&)(int)>)
        return "void(&)(int)";

    // 成员指针类型
    if constexpr (std::is_same_v<T, int MyStruct::*>)
        return "int MyStruct::*";
    if constexpr (std::is_same_v<T, void (MyStruct::*)() const>)
        return "void (MyStruct::*)() const";

    // 数组类型
    if constexpr (std::is_same_v<T, int[10]>)
        return "int[10]";
    if constexpr (std::is_same_v<T, const char[15]>)
        return "const char[15]";

    // 标准库类型
    if constexpr (std::is_same_v<T, std::vector<int>>)
        return "std::vector<int>";
    if constexpr (std::is_same_v<T, std::vector<int> &>)
        return "std::vector<int>&";
    if constexpr (std::is_same_v<T, const std::vector<int> &>)
        return "const std::vector<int>&";
    if constexpr (std::is_same_v<T, std::vector<int> &&>)
        return "std::vector<int>&&";

    // 添加更多类型匹配...

    return "unknown";
}

// 打印类型信息的辅助函数
template <typename T>
void print_type_info(const char *name)
{
    std::cout << "  " << name << " = " << type_name<T>() << "\n";
}

// 测试函数
template <typename T>
void run_test(T &&value, const char *category)
{
    std::cout << "\n=== " << category << " ===\n";
    auto types = test_template_type(std::forward<T>(value));

    print_type_info<typename decltype(types)::t0>("T0");
    print_type_info<typename decltype(types)::t1>("T1");
    print_type_info<typename decltype(types)::t2>("T2");
    print_type_info<typename decltype(types)::t3>("T3");
    print_type_info<typename decltype(types)::t4>("T4");
    print_type_info<typename decltype(types)::t5>("T5");
    print_type_info<typename decltype(types)::t6>("T6");
    print_type_info<typename decltype(types)::t7>("T7");
    print_type_info<typename decltype(types)::t8>("T8");
    print_type_info<typename decltype(types)::t9>("T9");
    print_type_info<typename decltype(types)::t10>("T10");
}

int main()
{
    // 基本类型测试
    int x = 42;
    const int cx = 100;
    volatile int vx = 200;
    const volatile int cvx = 300;

    run_test(x, "int lvalue");
    run_test(cx, "const int lvalue");
    run_test(vx, "volatile int lvalue");
    run_test(cvx, "const volatile int lvalue");
    run_test(42, "int rvalue");
    run_test(std::move(x), "int xvalue");

    // 自定义类型测试
    MyStruct s;
    const MyStruct cs{};

    run_test(s, "MyStruct lvalue");
    run_test(cs, "const MyStruct lvalue");
    run_test(MyStruct{}, "MyStruct rvalue");
    run_test(std::move(s), "MyStruct xvalue");

    // 引用类型测试
    int &rx = x;
    const int &crx = x;
    int &&rrx = 42;

    run_test(rx, "int&");
    run_test(crx, "const int&");
    run_test(rrx, "int&&");
    run_test(std::move(rrx), "int&& moved");

    // 函数类型测试
    run_test(test_function, "function lvalue");
    run_test(&test_function, "function pointer");

    // 成员指针测试
    int MyStruct::*mem_ptr = &MyStruct::value;
    run_test(mem_ptr, "member pointer");

    void (MyStruct::*mem_fn_ptr)() const = &MyStruct::method;
    run_test(mem_fn_ptr, "member function pointer");

    // 数组类型测试
    int arr[10]{};
    run_test(arr, "array lvalue");
    run_test("string literal", "string literal");

    // 复杂类型测试
    std::vector<int> vec{1, 2, 3};
    const std::vector<int> cvec{4, 5, 6};

    run_test(vec, "vector lvalue");
    run_test(cvec, "const vector lvalue");
    run_test(std::vector<int>{7, 8, 9}, "vector rvalue");
    run_test(std::move(vec), "vector xvalue");

    // NOTE: 结论：完美转发，不会丢失 cvrf.  forward_like 对局部变量一样完美转发
    {
        auto &&r_v = 0;
        auto &l_v = r_v;
        static_assert(std::is_same_v<decltype(r_v), int &&>);
        static_assert(std::is_same_v<decltype(l_v), int &>);

        static_assert(
            std::is_same_v<decltype(r_v), decltype(std::forward<decltype(r_v)>(r_v))>);
        static_assert(
            std::is_same_v<decltype(l_v), decltype(std::forward<decltype(l_v)>(l_v))>);
        static_assert(std::is_same_v<decltype(r_v),
                                     decltype(std::forward_like<decltype(r_v)>(r_v))>);
        static_assert(std::is_same_v<decltype(l_v),
                                     decltype(std::forward_like<decltype(l_v)>(l_v))>);

        {
            // NOTE: forward_like 会将输入类型 转换为目标类型
            auto &&fv =
                std::forward_like<decltype(l_v)>(std::forward<decltype(r_v)>(r_v));
            static_assert(
                std::is_rvalue_reference_v<decltype(std::forward<decltype(r_v)>(r_v))>);
            static_assert(std::is_lvalue_reference_v<decltype(fv)>);

            // NOTE: 左值引用 转为 右值 引用
            auto &&fv2 =
                std::forward_like<decltype(r_v)>(std::forward<decltype(l_v)>(l_v));
            static_assert(std::is_rvalue_reference_v<decltype(fv2)>);
            static_assert(
                std::is_lvalue_reference_v<decltype(std::forward<decltype(l_v)>(l_v))>);
        }

        static_assert(std::is_same_v<decltype((l_v)), int &>); // NOTE 不变

        // NOTE: decltype((xxx)) ，如果 xxx 是局部变量，那么总是左值
        static_assert(std::is_same_v<decltype((r_v)), int &>);

        static_assert(std::is_reference_v<decltype(r_v)>);
        static_assert(std::is_rvalue_reference_v<decltype(r_v)>);

        static_assert(not std::is_rvalue_reference_v<decltype((r_v))>);
        static_assert(not std::is_rvalue_reference_v<decltype((0))>);
        static_assert(not std::is_rvalue_reference_v<decltype((auto{0}))>);
        static_assert(not std::is_rvalue_reference_v<decltype(0)>);
        static_assert(not std::is_rvalue_reference_v<decltype(auto{0})>);

        // NOTE: decltype((xxx)) 可以 既不是左值，也不是右值
        static_assert(std::is_same_v<decltype((0)), int>);
        static_assert(std::is_same_v<decltype((auto{0})), int>);
        static_assert(not std::is_same_v<decltype((auto{0})), decltype((r_v))>);

        // NOTE: 左值、右值是对局部变量命名的。直接值，按值语义解释，有类型没有引用修饰
    }
    {
        // NOTE: decltype((xxx))
        auto i = 0;
        static_assert(std::is_same_v<decltype((i)), int &>);
        static_assert(std::is_same_v<decltype((std::move(i))), int &&>);
        static_assert(std::is_same_v<decltype((std::move(0))), int &&>);
        static_assert(std::is_same_v<decltype((std::move(auto{0}))), int &&>);

        static_assert(std::is_same_v<decltype((0)), int>);
        static_assert(std::is_same_v<decltype((auto{0})), int>);

        const auto &ci = i;
        static_assert(std::is_same_v<decltype((ci)), const int &>);

        auto &&mi = std::move(i);
        static_assert(std::is_same_v<decltype((mi)), int &>);
        static_assert(std::is_same_v<decltype((std::move(i))), int &&>);
    }
    {
        // 即使 T0 != T2。 转发还是一样的
        int v = 0;
        using T0 = decltype(std::forward<int>(v));
        using T1 = decltype(std::forward<int &&>(v));
        using T2 = decltype(std::forward_like<int>(v));
        using T3 = decltype(std::forward_like<int &&>(v));
        static_assert(std::is_same_v<T0, T1> && std::is_same_v<T1, T2> &&
                      std::is_same_v<T2, T3>);
        static_assert(std::is_same_v<T0, int &&>);

        {
            int i = 0;
            int &v = i;
            using T0 = decltype(std::forward<int>(v));
            using T1 = decltype(std::forward<int &&>(v));
            using T2 = decltype(std::forward_like<int>(v));
            using T3 = decltype(std::forward_like<int &&>(v));
            static_assert(std::is_same_v<T0, T1> && std::is_same_v<T1, T2> &&
                          std::is_same_v<T2, T3>);
            static_assert(std::is_same_v<T3, int &&>);
        }
        {
            int i = 0;
            int &&v = std::move(i);
            using T0 = decltype(std::forward<int>(v));
            using T1 = decltype(std::forward<int &&>(v));
            using T2 = decltype(std::forward_like<int>(v));
            using T3 = decltype(std::forward_like<int &&>(v));
            static_assert(std::is_same_v<T0, T1> && std::is_same_v<T1, T2> &&
                          std::is_same_v<T2, T3>);
            static_assert(std::is_same_v<T3, int &&>);
        }
        {
            int i = 0;
            const int &v = i;
            // using T0 = decltype(std::forward<int>(v)); //NOTE: 编译错误
            // using T1 = decltype(std::forward<int &&>(v));
            using T2 = decltype(std::forward_like<int>(v));
            using T3 = decltype(std::forward_like<int &&>(v));
            static_assert(std::is_same_v<T0, T1> && std::is_same_v<T2, T3>);

            // NOTE: like 的意思是 补全 const. 补全 cv
            static_assert(std::is_same_v<T3, const int &&>);
        }
        {
            int i = 0;
            const int &&v = std::move(i);
            // using T0 = decltype(std::forward<int>(v)); //NOTE: 编译错误
            // using T1 = decltype(std::forward<int &&>(v));
            using T2 = decltype(std::forward_like<int>(v));
            using T3 = decltype(std::forward_like<int &&>(v));
            static_assert(std::is_same_v<T0, T1> && std::is_same_v<T2, T3>);

            // NOTE: like 的意思是 补全 const. 补全 cv
            static_assert(std::is_same_v<T3, const int &&>);

            auto fun = [](int &&v) {
            };
            // fun(std::forward_like<int &&>(v)); // 编译错误

            static auto call = -1;
            struct Test
            {
                int value;

                // 普通构造函数
                Test(int v) : value(v)
                {
                    std::cout << "普通构造函数\n";
                }

                // 复制构造函数
                Test(const Test &other) : value(other.value)
                {
                    call = 0;
                    std::cout << "复制构造函数\n";
                }

                // 移动构造函数（非const右值引用）
                Test(Test &&other) noexcept : value(other.value)
                {
                    call = 1;
                    other.value = -1; // 修改源对象，标记为已移动
                    std::cout << "移动构造函数（非const参数）\n";
                }

                // 尝试定义接受const右值引用的移动构造函数。//NOTE: 不是特殊函数
                // Test(const Test &&other) noexcept : value(other.value)
                // {
                //     // 无法修改other，因为它是const的
                //     std::cout << "移动构造函数（const参数）\n";
                // }
            };

            {
                const auto v = Test{0};
                const auto &&t0 = std::move(v);
                auto obj{std::forward_like<Test &&>(v)};
                assert(call == 0); // const 确定了 走复制

                // NOTE: 确定了 const T& 能接受任意类型输入，值语义，复制
            }
        }

        // NOTE: 毫无疑问 forward_like 更强。 更不容易编译错误
    }
    std::cout << "\n=== Testing complete ===\n";
    return 0;
}
// NOLINTEND