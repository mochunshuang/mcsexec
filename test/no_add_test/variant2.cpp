#include <cassert>
#include <iostream>
#include <variant>

struct a_class
{
    a_class() = delete;
    explicit a_class(int v) noexcept : v{v} {};
    int v{}; // NOLINT
};

int main()
{
    // NOTE: variant 允许不能默认初始化
    using T = std::variant<a_class>; // 允许
    // T a{};//错误
    auto ret = [](int v) noexcept {
        return T{a_class{v}};
    };
    auto r = ret(2);
    assert(r.index() == 0);
    std::cout << "main done\n";
    return 0;
}