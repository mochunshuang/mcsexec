#include <array>
#include <format>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

template <typename... Args>
std::string dyna_print(std::string_view rt_fmt_str, Args &&...args)
{
    return std::vformat(rt_fmt_str, std::make_format_args(args...));
}

// 一个简单的例子，演示一种思路（实际应用可能更复杂）
template <std::size_t N>
struct ConstexprFormatString
{
    constexpr ConstexprFormatString(const char (&chars)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            data[i] = chars[i];
        }
    }
    char data[N] = {};
    static constexpr std::size_t size = N;
    constexpr operator std::string_view() const
    {
        return {data, size - 1};
    } // 忽略末尾'\0'
};

int main()
{
#ifdef __cpp_lib_format_ranges
    const std::set<std::string_view> continents{"Africa", "America",   "Antarctica",
                                                "Asia",   "Australia", "Europe"};
    std::cout << std::format("Hello {}!\n", continents);
#else
    std::cout << std::format("Hello {}!\n", "continents");
#endif

    std::string fmt;
    for (int i{}; i != 3; ++i)
    {
        fmt += "{} "; // constructs the formatting string
        std::cout << fmt << " : ";
        std::cout << dyna_print(fmt, "alpha", 'Z', 3.14, "unused");
        std::cout << '\n';
    }

    {
        constexpr auto a = "asdsa"
                           "bcd";

        // constexpr auto b = a
        //                    "bcd";
    }
    {
        std::format(std::format_string<int>{""}, 0);
        constexpr std::array<char, 3> src = {'a', '{', '}'};
        // std::format(std::format_string<int>{std::string_view{src, src.size()}}, 0);
    }
    {
        // 在 constexpr 上下文中“准备”你的字符串
        constexpr ConstexprFormatString fmt_arr = {"a{}"}; // 用一个字面量初始化它
        // 然后使用这个转换得到的 string_view
        // std::cout << std::format(std::format_string<int>{fmt_arr}, 0)
        //           << std::endl; // 输出: a0
    }
}