#include <cstdint>
#include <format>
#include <iostream>
#include <source_location>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <stacktrace>

// NOLINTBEGIN
struct mcslog
{
    enum class LOG_LEVEL : std::uint8_t
    {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        END
    };
    // NOLINTBEGIN
    // 经典主题 (推荐)
    struct classic_theme
    {
        static constexpr const char *colors[] = {
            "\033[37m",     // TRACE: 白色
            "\033[36m",     // DEBUG: 青色
            "\033[32m",     // INFO:  绿色
            "\033[33m",     // WARN:  黄色
            "\033[31m",     // ERROR: 红色
            "\033[1;41;37m" // FATAL: 白字红底粗体 (最醒目)
        };
    };

    // 专业主题
    struct professional_theme
    {
        static constexpr const char *colors[] = {
            "\033[2;37m",   // TRACE: 淡灰色
            "\033[34m",     // DEBUG: 蓝色
            "\033[32m",     // INFO:  绿色
            "\033[33m",     // WARN:  黄色
            "\033[35m",     // ERROR: 紫色
            "\033[1;45;37m" // FATAL: 白字紫红底粗体
        };
    };

    // 明亮主题
    struct bright_theme
    {
        static constexpr const char *colors[] = {
            "\033[90m",      // TRACE: 灰色
            "\033[96m",      // DEBUG: 亮青色
            "\033[92m",      // INFO:  亮绿色
            "\033[93m",      // WARN:  亮黄色
            "\033[91m",      // ERROR: 亮红色
            "\033[1;101;97m" // FATAL: 亮白字亮红底粗体
        };
    };

    // 简约主题 (只突出重要级别)
    struct minimal_theme
    {
        static constexpr const char *colors[] = {
            "\033[0m",     // TRACE: 默认色
            "\033[0m",     // DEBUG: 默认色
            "\033[32m",    // INFO:  绿色
            "\033[33m",    // WARN:  黄色
            "\033[31m",    // ERROR: 红色
            "\033[1;5;31m" // FATAL: 红色粗体闪烁 (非常醒目)
        };
    };
    // 复古主题 (终端经典风格)
    struct retro_theme
    {
        static constexpr const char *colors[] = {
            "\033[0m",       // TRACE: 默认色
            "\033[0m",       // DEBUG: 默认色
            "\033[0m",       // INFO:  默认色
            "\033[33m",      // WARN:  黄色
            "\033[31m",      // ERROR: 红色
            "\033[1;5;7;31m" // FATAL: 反白闪烁红色粗体 (最经典醒目)
        };
    };
    static constexpr const char *const *const all_theme[] = {
        classic_theme::colors, professional_theme::colors, bright_theme::colors,
        minimal_theme::colors, retro_theme::colors};
    static constexpr std::string_view log_level_name[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"}; // NOLINTNEXTLINE
    static constexpr auto FUN_COLOR = "\033[38;2;255;165;0m";
    static constexpr auto RESET_COLOR = "\033[0m";
    static constexpr auto THEME_INDEX = 0;
    static constexpr auto ENABLE_LOG = true;
    static constexpr auto ENABLE_TRACE = true;
    static constexpr auto ENABLE_DEBUG = true;
    static constexpr auto ENABLE_INFO = true;
    static constexpr auto ENABLE_WARN = true;
    static constexpr auto ENABLE_ERROR = true;
    static constexpr auto ENABLE_FATAL = true;

    // NOLINTEND
    static constexpr void print_color(
        std::source_location source = std::source_location::current())
    {
        for (const auto &theme : all_theme)
        {
            for (int i = 0; i < static_cast<uint8_t>(LOG_LEVEL::END); ++i)
            {
                std::println("{}", std::format("{}[{:^7}] [{}:{}:{}{}{}]: {}{}", theme[i],
                                               log_level_name[i], source.file_name(),
                                               source.line(), FUN_COLOR,
                                               source.function_name(), theme[i],
                                               std::string{"hello msg..."}, RESET_COLOR));
            }
            std::println();
        }
    }

    static constexpr auto &current_log_level() noexcept
    {
        static LOG_LEVEL level = LOG_LEVEL::TRACE;
        return level;
    }
    static constexpr void set_log_level(LOG_LEVEL level) noexcept
    {
        current_log_level() = level;
    }

    static constexpr auto log(const std::string &msg, std::source_location source =
                                                          std::source_location::current())
    {
        std::println("[{}:{}:{}]: {}", source.file_name(), source.line(),
                     source.function_name(), msg);
    }

    template <typename... Args>
    static constexpr void trace(const std::source_location &source,
                                std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (ENABLE_LOG and ENABLE_TRACE)
        {
            constexpr auto current_level = LOG_LEVEL::TRACE;
            constexpr auto log_level_index = static_cast<uint8_t>(current_level);
            if (current_level >= current_log_level())
                std::println("{}[{:^7}] [{}:{}:{}{}{}]: {}{}",
                             all_theme[THEME_INDEX][log_level_index],
                             log_level_name[log_level_index], source.file_name(),
                             source.line(), FUN_COLOR, source.function_name(),
                             all_theme[THEME_INDEX][log_level_index],
                             std::format(fmt, std::forward<Args>(args)...), RESET_COLOR);
        }
    }
    template <typename... Args>
    static constexpr void debug(const std::source_location &source,
                                std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (ENABLE_LOG and ENABLE_DEBUG)
        {
            constexpr auto current_level = LOG_LEVEL::DEBUG;
            constexpr auto log_level_index = static_cast<uint8_t>(current_level);
            if (current_level >= current_log_level())
                std::println("{}[{:^7}] [{}:{}:{}{}{}]: {}{}",
                             all_theme[THEME_INDEX][log_level_index],
                             log_level_name[log_level_index], source.file_name(),
                             source.line(), FUN_COLOR, source.function_name(),
                             all_theme[THEME_INDEX][log_level_index],
                             std::format(fmt, std::forward<Args>(args)...), RESET_COLOR);
        }
    }

    template <typename... Args>
    static constexpr void info(const std::source_location &source,
                               std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (ENABLE_LOG and ENABLE_INFO)
        {
            constexpr auto current_level = LOG_LEVEL::INFO;
            constexpr auto log_level_index = static_cast<uint8_t>(current_level);
            if (current_level >= current_log_level())
                std::println("{}[{:^7}] [{}:{}:{}{}{}]: {}{}",
                             all_theme[THEME_INDEX][log_level_index],
                             log_level_name[log_level_index], source.file_name(),
                             source.line(), FUN_COLOR, source.function_name(),
                             all_theme[THEME_INDEX][log_level_index],
                             std::format(fmt, std::forward<Args>(args)...), RESET_COLOR);
        }
    }

    template <typename... Args>
    static constexpr void warn(const std::source_location &source,
                               std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (ENABLE_LOG and ENABLE_WARN)
        {
            constexpr auto current_level = LOG_LEVEL::WARN;
            constexpr auto log_level_index = static_cast<uint8_t>(current_level);
            if (current_level >= current_log_level())
                std::println("{}[{:^7}] [{}:{}:{}{}{}]: {}{}",
                             all_theme[THEME_INDEX][log_level_index],
                             log_level_name[log_level_index], source.file_name(),
                             source.line(), FUN_COLOR, source.function_name(),
                             all_theme[THEME_INDEX][log_level_index],
                             std::format(fmt, std::forward<Args>(args)...), RESET_COLOR);
        }
    }
    template <typename... Args>
    static constexpr void error(const std::source_location &source,
                                std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (ENABLE_LOG and ENABLE_ERROR)
        {
            constexpr auto current_level = LOG_LEVEL::ERROR;
            constexpr auto log_level_index = static_cast<uint8_t>(current_level);
            if (current_level >= current_log_level())
                std::println("{}[{:^7}] [{}:{}:{}{}{}]: {}{}",
                             all_theme[THEME_INDEX][log_level_index],
                             log_level_name[log_level_index], source.file_name(),
                             source.line(), FUN_COLOR, source.function_name(),
                             all_theme[THEME_INDEX][log_level_index],
                             std::format(fmt, std::forward<Args>(args)...), RESET_COLOR);
        }
    }

    template <typename... Args>
    static constexpr void fatal(const std::source_location &source,
                                std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (ENABLE_LOG and ENABLE_FATAL)
        {
            constexpr auto current_level = LOG_LEVEL::FATAL;
            constexpr auto log_level_index = static_cast<uint8_t>(current_level);
            std::println("{}[{:^7}] [{}:{}:{}{}{}]: {}{}",
                         all_theme[THEME_INDEX][log_level_index],
                         log_level_name[log_level_index], source.file_name(),
                         source.line(), FUN_COLOR, source.function_name(),
                         all_theme[THEME_INDEX][log_level_index],
                         std::format(fmt, std::forward<Args>(args)...), RESET_COLOR);
        }
    }
};

template <typename... As>
void print(As &&...as)
{
    mcslog::print_color();
}

#define MCSLOG_LOG(...) ::mcslog::log(__VA_ARGS__)
#define MCSLOG_TRACE(...) ::mcslog::trace(std::source_location::current(), __VA_ARGS__)
#define MCSLOG_DEBUG(...) ::mcslog::debug(std::source_location::current(), __VA_ARGS__)
#define MCSLOG_INFO(...) ::mcslog::info(std::source_location::current(), __VA_ARGS__)
#define MCSLOG_WARN(...) ::mcslog::warn(std::source_location::current(), __VA_ARGS__)
#define MCSLOG_ERROR(...) ::mcslog::error(std::source_location::current(), __VA_ARGS__)
#define MCSLOG_FATAL(...) ::mcslog::fatal(std::source_location::current(), __VA_ARGS__)

void do_throw(auto &&...v)
{
    // NOTE: 建议是 储存 stacktrace 对象。然后打印。肯定是找不到哪里出现的错误
    // NOTE: 总之肯定是不如 log 详细
    std::cout << std::stacktrace::current() << '\n';
    throw std::runtime_error{"test throw..."};
}

template <typename... As>
void with_stacktrace(As &&...as)
{
    do_throw(std::forward<As>(as)...);
}

int main()
{
    print("123", 1, std::string{});
    mcslog::debug(std::source_location::current(), "{}", "hello");
    MCSLOG_DEBUG("{}", "hello");
    std::println();
    mcslog::trace(std::source_location::current(), "{}", "log msg...");
    mcslog::debug(std::source_location::current(), "{}", "log msg...");
    mcslog::info(std::source_location::current(), "{}", "log msg...");
    mcslog::warn(std::source_location::current(), "{}", "log msg...");
    mcslog::error(std::source_location::current(), "{}", "log msg...");
    mcslog::fatal(std::source_location::current(), "{}", "log msg...");
    std::println();

    mcslog::set_log_level(mcslog::LOG_LEVEL::INFO);
    MCSLOG_TRACE("{}", "hello");
    MCSLOG_DEBUG("{}", "hello");
    MCSLOG_INFO("{}", "hello"); // NOTE: INFO 以上的将不打印
    MCSLOG_WARN("{}", "hello");
    MCSLOG_ERROR("{}", "hello");
    MCSLOG_FATAL("{}", "hello");

    std::println();
    try
    {
        with_stacktrace("1", 1, 3.0);
    }
    catch (const std::exception &e)
    {
        MCSLOG_ERROR("[what]: {}\n[stacktrace]:\n{}", e.what(),
                     std::stacktrace::current());
    }

    std::println("{}", nullptr);
    const char *chr = nullptr;
    // std::println("{}", chr); //NOTE: 崩溃
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND