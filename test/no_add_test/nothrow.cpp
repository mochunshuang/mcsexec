#include <type_traits>

// NOLINTBEGIN
// 测试工具

template <bool Expected, typename F, typename... Args>
void test() noexcept // NOLINT
{
    static_assert(Expected == std::is_nothrow_invocable_v<F, Args...>);
}

// 测试用例定义
void may_throw()
{
    throw 1;
}
void no_throw() noexcept {}

struct ThrowFunctor
{
    void operator()()
    {
        throw 1;
    }
};
struct NothrowFunctor
{
    void operator()() noexcept {}
};

struct MyClass
{
    void throw_mem()
    {
        throw 1;
    }
    void nothrow_mem() noexcept {}
    static void static_throw()
    {
        throw 1;
    }
    static void static_nothrow() noexcept {}
};
// NOLINTEND

int main()
{
    // 普通函数
    test<false, decltype(&may_throw)>();
    test<true, decltype(&no_throw)>();

    // 函数指针
    // using FuncPtr = void (*)();
    // test<false, FuncPtr>(&may_throw);
    // test<true, FuncPtr>(&no_throw);
    // 需要通过 static_cast 明确 noexcept 属性，比较麻烦
    test<false, decltype(static_cast<void (*)()>(may_throw))>();
    test<true, decltype(static_cast<void (*)() noexcept>(no_throw))>();

    // 仿函数对象
    test<false, ThrowFunctor>();
    test<true, NothrowFunctor>();

    // Lambda 表达式
    test<false, decltype([] { throw 1; })>(); // NOLINT
    test<true, decltype([]() noexcept {})>();

    // 成员函数
    test<false, decltype(&MyClass::throw_mem), MyClass *>();
    test<true, decltype(&MyClass::nothrow_mem), MyClass *>();

    // 静态成员函数
    test<false, decltype(&MyClass::static_throw)>();
    test<true, decltype(&MyClass::static_nothrow)>();

    // 带参数的测试
    struct ArgTester
    {
        void operator()(int /*unused*/) noexcept {}
        void operator()(double /*unused*/)
        {
            throw 1; // NOLINT
        }
    };
    test<true, ArgTester, int>();
    test<false, ArgTester, double>();

    // NOTE: 结论，知道函数的所有类型，就能算出 是否有异常。返回值更简单了

    return 0;
}