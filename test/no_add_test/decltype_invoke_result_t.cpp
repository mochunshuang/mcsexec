#include <type_traits>
#include <iostream>
#include <utility>
// NOLINTBEGIN
// 场景1: 普通函数
int normal_func(double x)
{
    return static_cast<int>(x);
}

// 场景2: 重载函数
int overloaded(int x)
{
    return x;
}
double overloaded(double x)
{
    return x;
}

// 场景4: 成员函数测试
struct MyClass
{
    int member_func(double)
    {
        return 42;
    }
};

// 场景5: 模板元编程辅助类型
template <typename F, typename Arg>
using ResultTypeInvoke = std::invoke_result_t<F, Arg>;

template <typename F, typename Arg>
using ResultTypeDecl = decltype(std::declval<F>()(std::declval<Arg>()));

template <typename F>
decltype(auto) template_fun(F &&v)
{
    return std::forward<F>(v);
}

int main()
{

    //  场景1: 普通函数
    static_assert(std::is_same_v<decltype(normal_func(1.0)), int>,
                  "Test 1.1 failed"); // ✅
    static_assert(
        std::is_same_v<std::invoke_result_t<decltype(normal_func), double>, int>,
        "Test 1.2 failed"); // ✅

    // 场景2: 重载函数
    static_assert(std::is_same_v<decltype(overloaded(1.0)), double>,
                  "Test 2.1 failed"); // ✅ 根据参数类型推导
    // static_assert(
    //     std::is_same_v<std::invoke_result_t<decltype(overloaded), double>, double>,
    //     "Test 2.2 failed"); // ❌ 无法推导重载函数类型

    // 场景3: 泛型Lambda
    auto generic_lambda = [](auto x) {
        return x;
    };
    static_assert(std::is_same_v<decltype(generic_lambda(42)), int>,
                  "Test 3.1 failed"); // ✅
    static_assert(
        std::is_same_v<std::invoke_result_t<decltype(generic_lambda), int>, int>,
        "Test 3.2 failed"); // ✅

    // 场景4: 成员函数指针
    int (MyClass::*mem_ptr)(double) = &MyClass::member_func;
    MyClass obj;
    static_assert(std::is_same_v<decltype((obj.*mem_ptr)(1.0)), int>,
                  "Test 4.1 failed"); // ✅
    static_assert(
        std::is_same_v<std::invoke_result_t<decltype(mem_ptr), MyClass *, double>, int>,
        "Test 4.2 failed"); // ✅

    // 场景5: 模板元编程
    static_assert(std::is_same_v<ResultTypeInvoke<decltype(normal_func), double>, int>,
                  "Test 5.1 failed"); // ✅
    static_assert(std::is_same_v<ResultTypeDecl<decltype(normal_func), double>, int>,
                  "Test 5.2 failed"); // ✅

    // 大区别
    {
        int x = 42;

        // 使用 std::invoke_result_t 推导 template_fun 的返回类型
        using ResultType = std::invoke_result_t<decltype(template_fun<int &>), int &>;

        // 简化和不简化的
        static_assert(
            std::is_same_v<ResultType, decltype(template_fun(std::declval<int &>()))>);
        static_assert(
            std::is_same_v<ResultType,
                           decltype(std::declval<decltype(template_fun<int &>)>()(
                               std::declval<int &>()))>);
    }

    // 场景6: noexcept检测
    auto noexcept_lambda = [](int) noexcept -> int {
        return 0;
    };
    auto throwing_lambda = [](double) -> int {
        return 1;
    };
    static_assert(noexcept(noexcept_lambda(42)), "Test 6.1 failed");   // ✅
    static_assert(!noexcept(throwing_lambda(1.0)), "Test 6.2 failed"); // ✅

    std::cout << "All tests passed!\n";
    return 0;
}
// 针对代码膨胀问题

// 示例说明：
auto fun = [](const int &v) -> const int & {
    return v;
};
using Fun_t = decltype(fun);

// 使用 decltype：直接推导，需要 declval 开销。开销很小的
using R_decltype = decltype(std::declval<Fun_t>()(std::declval<int>()));
static_assert(std::is_same_v<R_decltype, const int &>);

// 使用 invoke_result_t：依赖模板实例化
using R_invoke = std::invoke_result_t<Fun_t, int &>;
static_assert(std::is_same_v<R_invoke, const int &>);

// NOLINTEND