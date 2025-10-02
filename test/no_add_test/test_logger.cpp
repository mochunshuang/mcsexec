
#include <format>
#include <iostream>
#include <string>
#include <string_view>
// NOLINTBEGIN
constexpr auto test(const char (&fmt)[2], const char (&pre)[2], const char (&post)[2])
{
    constexpr char data[3] = {'a', 'b', 'c'};
    return std::string_view{data};
}

int main()
{
    auto fmt = test("a", "b", "c");
    {
        // constexpr auto fmt = test("a", "b", "c"); // 编译错误
    }
    // std::vformat(fmt); //不行
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND