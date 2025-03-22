#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <source_location>
#include <string_view>
#include <iostream>

#include <sstream>
#include <utility>

namespace mcstest
{
    struct test_counter
    {
        std::atomic<std::size_t> pass_count;  // NOLINT
        std::atomic<std::size_t> total_count; // NOLINT

        void print() const noexcept
        {

            std::cout << "\033[32m"; // 开始绿色输出
            std::cout << "total_assert: " << total_count << ", ";
            std::cout << "pass_assert: " << pass_count << '\n';
            std::cout << "\033[0m"; // 重置颜色到默认设置
        }
    };

    static auto &get_test_counter() // NOLINT
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

    class ExpectError : public std::exception
    {
      public:
        explicit ExpectError(std::string msg) : m_message(std::move(msg)) {}
        [[nodiscard]] const char *what() const noexcept override
        {
            return m_message.c_str();
        }

      private:
        std::string m_message;
    };

    // NOLINTNEXTLINE
    static std::string formatErrorMessage(const std::source_location &location) noexcept
    {
        std::stringstream ss;
        ss << "file_name: " << location.file_name() << ", line: " << location.line()
           << ", column: " << location.column();

        if (location.function_name() == nullptr)
        {
            ss << ", function_name: " << location.function_name();
        }
        ss << "\n";
        return ss.str();
    }

    auto expect(const bool &expr, // NOLINT
                const std::source_location &s = std::source_location::current()) -> void
    {
        if (expr)
            get_test_counter().pass_count++;
        else
        {
            throw ExpectError(formatErrorMessage(s));
        }
        get_test_counter().total_count++;
    }
    static auto unexpect( // NOLINT
        std::string_view msg,
        const std::source_location &s = std::source_location::current()) noexcept
    {
        std::cout << "unexpect: " << msg << " ,line: " << s.line();
        std::abort();
    }

    struct Test
    {
        test_info info; // NOLINT

        auto &operator=(auto &&test)
        {
            try
            {
                test();
            }
            catch (const ExpectError &e)
            {
                std::cout << "testname: " << info.name << " fault: " << e.what();
                throw;
            }
            return *this;
        }
    };

    // NOLINTNEXTLINE
    static auto add_test(
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