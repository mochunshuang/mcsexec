#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <source_location>
#include <string_view>
#include <iostream>
#include <format>

namespace mcstest
{
    // ANSI 颜色控制码
    namespace AnsiColor // NOLINTBEGIN
    {
        constexpr const char *reset = "\033[0m";    // 重置所有样式
        constexpr const char *red = "\033[31m";     // 红色文本
        constexpr const char *green = "\033[32m";   // 绿色文本
        constexpr const char *yellow = "\033[33m";  // 黄色文本
        constexpr const char *blue = "\033[34m";    // 蓝色文本
        constexpr const char *magenta = "\033[35m"; // 品红色文本
        constexpr const char *cyan = "\033[36m";    // 青色文本
        constexpr const char *bold = "\033[1m";     // 粗体
    }; // namespace AnsiColor
    // NOLINTEND

    struct test_counter
    {
        std::atomic<std::size_t> pass_count;  // NOLINT
        std::atomic<std::size_t> total_count; // NOLINT

        constexpr void print() const noexcept
        {

            std::cout << AnsiColor::green; // 开始绿色输出
            std::cout << "total_assert: " << total_count << ", ";
            std::cout << "pass_assert: " << pass_count << '\n';
            std::cout << AnsiColor::reset; // 重置颜色到默认设置
        }
    };

    constexpr static auto &get_test_counter() noexcept // NOLINT
    {
        static test_counter cunter;
        struct print // NOLINT
        {
            ~print() noexcept
            {
                cunter.print();
            }
        };
        static print p;
        return cunter;
    }

    struct test_info
    {
        std::string_view name;         // NOLINT
        std::source_location location; // NOLINT
    };

    class ExpectError : public std::runtime_error
    {
      public:
        using runtime_error::runtime_error;
    };

    // NOLINTNEXTLINE
    constexpr static std::string formatErrorMessage(const std::source_location &location)
    {
        if (location.function_name() != nullptr)
            return std::format("file_name: {}, line: {}, column: {}, function_name: {}\n",
                               location.file_name(), location.line(), location.column(),
                               location.function_name());

        return std::format("file_name: {}, line: {}, column: {}\n", location.file_name(),
                           location.line(), location.column());
    }

    constexpr auto expect(const bool &expr, // NOLINT
                          const std::source_location &s = std::source_location::current())
        -> void
    {
        auto &count = get_test_counter();
        if (expr)
            count.pass_count++;
        else
            throw ExpectError(formatErrorMessage(s));
        count.total_count++;
    }
    constexpr static auto unexpect( // NOLINT
        std::string_view msg,
        const std::source_location &s = std::source_location::current()) noexcept
    {
        std::cout << "unexpect: " << msg << " ,line: " << s.line();
        std::abort();
    }

    struct Test
    {
        test_info info; // NOLINT

        constexpr auto &operator=(auto &&test)
        {
            try
            {
                test();
            }
            catch (const ExpectError &e)
            {
                std::cout
                    << AnsiColor::red << AnsiColor::bold
                    << "\n================[Assertion failed begin]================\n"
                    << AnsiColor::reset << AnsiColor::yellow
                    << "[testname]: " << info.name << "\n[info]: " << e.what()
                    << AnsiColor::red << AnsiColor::bold
                    << "\n================[Assertion failed end]================\n"
                    << AnsiColor::reset;
                ;
                throw;
            }
            return *this;
        }
    };

    // NOLINTNEXTLINE
    constexpr static auto add_test(
        std::string_view name,
        std::source_location location = std::source_location::current()) noexcept
    {
        return Test{.info = test_info{.name = name, .location = location}};
    }
}; // namespace mcstest

#define TEST(name) mcstest::add_test(name) // NOLINT
#define EXPECT(v) \
    mcstest::expect((v)) // NOLINT  ((v))确保传递的是一个完整的表达式而不是函数调用
#define UNEXPECT(message) mcstest::unexpect(message) // NOLINT