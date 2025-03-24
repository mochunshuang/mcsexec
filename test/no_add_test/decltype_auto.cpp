
#include <cassert>
#include <iostream>

// NOLINTBEGIN
constexpr decltype(auto) test()
{
    int a = 1;
    // warning: reference to local variable 'a' returned [-Wreturn-local-addr]
    return std::move(a);
}

constexpr decltype(auto) test2()
{
    return test();
}

constexpr auto test3()
{
    int a = 1;
    // warning: reference to local variable 'a' returned [-Wreturn-local-addr]
    return std::move(a);
}
constexpr decltype(auto) test4()
{
    return test3();
}

int main()
{
    // NOTE: 尽量不要返回引用
    [[maybe_unused]] auto &&ret = test2();
    // assert(ret == 1); // 崩溃
    static_assert(std::is_same_v<decltype(test4()), int>);
    static_assert(std::is_same_v<decltype(test2()), int &&>);

    static_assert(test4() == 1);
    // static_assert(test2() == 1); // 做不到
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND