#include <array>
#include <cassert>
#include <cstdio>
#include <format>
#include <iostream>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <source_location>
#include <utility>

// NOLINTBEGIN
namespace AnsiColor
{
    constexpr char reset[] = {"\033[0m"};
    constexpr char red[] = {"\033[31m"};
    constexpr char green[] = {"\033[32m"};
    constexpr char yellow[] = {"\033[33m"};
    constexpr char blue[] = {"\033[34m"};
    constexpr char magenta[] = {"\033[35m"};
    constexpr char cyan[] = {"\033[36m"};
    constexpr char bold[] = {"\033[1m"};
} // namespace AnsiColor

class Logger
{
  public:
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    static void set_level(Level level)
    {
        current_level = level;
    }

    // 修改函数声明，将source_location参数放在最后并添加默认值
    template <typename... Args>
    static void debug(std::format_string<Args...> fmt, Args &&...args)
    {
        log(Level::Debug, std::source_location::current(), fmt,
            std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(std::format_string<Args...> fmt, Args &&...args)
    {
        log(Level::Info, std::source_location::current(), fmt,
            std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warning(std::format_string<Args...> fmt, Args &&...args)
    {
        log(Level::Warning, std::source_location::current(), fmt,
            std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(std::format_string<Args...> fmt, Args &&...args)
    {
        log(Level::Error, std::source_location::current(), fmt,
            std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void critical(std::format_string<Args...> fmt, Args &&...args)
    {
        log(Level::Critical, std::source_location::current(), fmt,
            std::forward<Args>(args)...);
    }

  private:
    template <typename... Args>
    static void log(Level level, const std::source_location &loc,
                    std::format_string<Args...> fmt, Args &&...args)
    {
        if (level < current_level)
            return;

        auto level_str = [](Level level) -> std::string_view {
            switch (level)
            {
            case Level::Debug:
                return "DEBUG";
            case Level::Info:
                return "INFO";
            case Level::Warning:
                return "WARNING";
            case Level::Error:
                return "ERROR";
            case Level::Critical:
                return "CRITICAL";
            default:
                return "UNKNOWN";
            }
        }(level);

        auto color = [](Level level) noexcept -> std::string_view {
            switch (level)
            {
            case Level::Debug:
                return AnsiColor::cyan;
            case Level::Info:
                return AnsiColor::green;
            case Level::Warning:
                return AnsiColor::yellow;
            case Level::Error:
                return AnsiColor::red;
            case Level::Critical:
                return AnsiColor::magenta;
            default:
                return AnsiColor::reset;
            }
        }(level);

        // 输出带颜色的日志消息
        std::print("{}[{:>9}]{} {}:{}: ", color, level_str, AnsiColor::reset,
                   loc.file_name(), loc.line());
        std::println(fmt, std::forward<Args>(args)...);
    }

    static inline Level current_level = Level::Debug;
};

// 假设 C++26 中 std::string 完全 constexpr 化
consteval auto concat_str(const std::string &a, const std::string &b)
{
    return a + b; // 编译期执行拼接
}

void base()
{
    Logger::set_level(Logger::Level::Debug);

    Logger::debug("这是一条调试消息");
    Logger::info("这是一条信息消息，值为: {}", 42);
    Logger::warning("这是一条警告消息");
    Logger::error("这是一条错误消息");
    Logger::critical("这是一条严重错误消息");

    std::print(stdout, "{}\n", std::format("这是一条信息消息，值为: {}", 42));
    // std::print(stdout,
    //            std::runtime_format(std::string("这是一条信息消息，值为: {}") + '\n'),
    //            42);
    std::print(stdout, "{}\n", std::format("这是一条信息消息，值为: {}", 42));

    // constexpr auto s = concat_str("cpp", "26"); // 编译期结果为 "cpp26"
    // static_assert(concat_str("cpp", "26") == "cpp26");
}

#define WRAPPED_FORMAT(prefix, fmt, suffix) prefix fmt suffix

template <typename... Args>
void print_fmt(std::format_string<Args...> fmt, Args &&...args)
{
    std::println(fmt, std::forward<Args>(args)...);
}
template <std::size_t FmtLen, std::size_t PrefixLen, std::size_t SuffixLen>
consteval decltype(auto) make_string_view(const char (&fmt)[FmtLen],
                                          const char (&prefix)[PrefixLen],
                                          const char (&suffix)[SuffixLen])
{
    // 计算总长度（减去每个字符串的 null 终止符）
    constexpr std::size_t total_size = (PrefixLen - 1) + (FmtLen - 1) + (SuffixLen - 1);
    std::array<char, total_size + 1> result{}; // +1 用于 null 终止符
    std::size_t pos = 0;
    for (std::size_t i = 0; i < PrefixLen - 1; ++i)
    {
        result[pos++] = prefix[i];
    }

    for (std::size_t i = 0; i < FmtLen - 1; ++i)
    {
        result[pos++] = fmt[i];
    }
    for (std::size_t i = 0; i < SuffixLen - 1; ++i)
    {
        result[pos++] = suffix[i];
    }
    result[total_size] = '\0';
    return result;
}

template <std::size_t FmtLen, std::size_t PrefixLen, std::size_t SuffixLen>
consteval decltype(auto) make_string_view2(const char (&fmt)[FmtLen],
                                           const char (&prefix)[PrefixLen],
                                           const char (&suffix)[SuffixLen])
{
    // 计算总长度（减去每个字符串的 null 终止符）
    constexpr std::size_t total_size = (PrefixLen - 1) + (FmtLen - 1) + (SuffixLen - 1);

    // 使用 std::array 而不是局部字符数组
    std::array<char, total_size + 1> result{}; // +1 用于 null 终止符

    std::size_t pos = 0;

    // 复制前缀（不包括 null 终止符）
    for (std::size_t i = 0; i < PrefixLen - 1; ++i)
    {
        result[pos++] = prefix[i];
    }

    // 复制格式字符串（不包括 null 终止符）
    for (std::size_t i = 0; i < FmtLen - 1; ++i)
    {
        result[pos++] = fmt[i];
    }

    // 复制后缀（不包括 null 终止符）
    for (std::size_t i = 0; i < SuffixLen - 1; ++i)
    {
        result[pos++] = suffix[i];
    }

    // 添加 null 终止符
    result[total_size] = '\0';

    return result; // 返回整个数组
}

template <size_t N>
struct FixedString
{
    static constexpr size_t size = N;                             // NOLINT
    char value[N]{};                                              // NOLINT
    explicit constexpr FixedString(const char (&str)[N]) noexcept // NOLINT
    {
        std::copy_n(str, N, value);
    }
};

template <std::size_t N>
consteval std::string_view array_to_string_view(const std::array<char, N> &arr)
{
    return std::string_view(arr.data(), N - 1); // 去掉 null 终止符
}

template <typename... Args>
void print_fmt2(std::format_string<Args...> fmt, Args &&...args)
{
    std::println(fmt, std::forward<Args>(args)...);
}

// 使用示例
int main()
{

    base();
    // static_assert(wrap_format_string("a", "b", "c") == std::string_view{"bac"});
    // print_fmt("这是一条信息消息，值为: {}", 42);
    print_fmt(WRAPPED_FORMAT("[begin]", "这是一条信息消息，值为: {}", "[end]"), 42);
    print_fmt(WRAPPED_FORMAT("\033[34m", "这是一条信息消息，值为: {}", "\033[0m"), 42);

    constexpr auto s = make_string_view("a", "b", "c");
    std::cout << std::string_view{s} << "\n";
    std::cout << "start: \n";
    for (char c : s)
    {
        std::cout << c << " , ";
    }
    std::cout << "end\n";

    static_assert(array_to_string_view(make_string_view("a", "b", "c")) ==
                  std::string_view{"bac"});
    static_assert(array_to_string_view(make_string_view("这是一条信息消息，值为: {}",
                                                        "[begin]", "[end]")) ==
                  std::string_view{"[begin]这是一条信息消息，值为: {}[end]"});
    static_assert(array_to_string_view(make_string_view("这是一条信息消息，值为: {}",
                                                        "\033[34m", "\033[0m")) ==
                  std::string_view{"\033[34m这是一条信息消息，值为: {}\033[0m"});
    static_assert(std::string_view{"bac"}.size() == 3);

    static_assert(array_to_string_view(make_string_view("这是一条信息消息，值为: {}",
                                                        "\033[34m", "\033[0m")) ==
                  std::string_view{"\033[34m这是一条信息消息，值为: {}\033[0m"});

    {
        auto ss = std::format(std::string_view{"{}"}, 42);
        constexpr auto str_fmt = std::string_view{"{}"};
        ss = std::format(str_fmt, 42);
        // NOTE: 不是 常量表达式
        //  constexpr auto sv = array_to_string_view(
        //      make_string_view("这是一条信息消息，值为: {}", "\033[34m", "\033[0m"));

        // NOTE: 指针是无法还原的
        // NOTE: 因此; AnsiColor 的 成员必须是 [] 类型的字符串。标记不能丢失信息
        constexpr auto fmt_array = make_string_view2(
            AnsiColor::blue, "这是一条信息消息，值为: {}", AnsiColor::reset);

        {
        }
        std::cout << "ss: " << ss << '\n';
    }
    {
        
    }

    return 0;
}
// NOLINTEND