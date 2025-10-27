#include <iostream>

// NOLINTBEGIN
struct a_class
{
};

auto check(auto call)
{
    static_assert(std::is_same_v<decltype(call()), a_class>);
    std::cout << "check ok\n";
}

int main()
{
    auto fun = [] {
        return a_class{};
    };
    check(fun);

    auto fun2 = [] {
        return int{};
    };
    // check(fun2); //编译不通过

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND