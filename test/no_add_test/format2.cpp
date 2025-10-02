#include <print> // C++23 的 <print> 头文件
#include <source_location>
#include <string_view>
#include <chrono>

// NOLINTBEGIN
// ANSI 颜色代码
namespace ansi
{
    constexpr std::string_view reset = "\033[0m";
    constexpr std::string_view red = "\033[31m";
    constexpr std::string_view green = "\033[32m";
    constexpr std::string_view blue = "\033[34m";
    constexpr std::string_view yellow = "\033[33m";
} // namespace ansi

// 高性能彩色打印函数
template <typename... Args>
void print_colored(const std::string_view &color, std::format_string<Args...> fmt,
                   Args &&...args)
{

    // 然后一次性输出（颜色 + 内容 + 重置）
    std::println("{}{}{}", color, std::format(fmt, std::forward<Args>(args)...),
                 ansi::reset);
}

// 特定颜色的便捷函数
template <typename... Args>
void print_red(std::format_string<Args...> fmt, Args &&...args)
{
    print_colored(ansi::red, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void print_green(std::format_string<Args...> fmt, Args &&...args)
{
    print_colored(ansi::green, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void log(std::source_location location, std::format_string<Args...> fmt,
                   Args &&...args)
{
    std::println("{}[{}] [{}:{}] {}{}", ansi::green,
                 std::chrono::current_zone()->to_local(std::chrono::system_clock::now()),
                 location.file_name(), location.line(),
                 std::format(fmt, std::forward<Args>(args)...), ansi::reset);
}

int main()
{
    int value = 42;
    std::string name = "World";

    // 使用高性能彩色打印
    print_red("Error: Value is {}!", value);
    print_green("Hello, {}!", name);

    // 直接使用 std::print（无颜色）
    std::print("The answer is {:.2f}\n", 3.14159);

    log(std::source_location::current(), "Error: Value is {}!", 24);
    return 0;
}
// NOLINTEND