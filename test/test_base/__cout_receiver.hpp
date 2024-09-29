#pragma once
#include "../../include/execution.hpp"
#include <iostream>

template <typename T>
concept has_print = requires(const T &obj) { obj.print(); };

struct cout_receiver
{
    using receiver_concept = mcs::execution::receiver_t;
    // Note: noexcept
    template <typename T>
    void set_value(T &&val) noexcept // NOLINT
    {
        std::cout << " cout_receiver: receiver: start" << '\n';
        if constexpr (requires(T &&val) {
                          std::declval<T>().begin();
                          std::declval<T>().end();
                      })
        {
            for (const auto &obj : val)
            {
                if constexpr (has_print<decltype(obj)>)
                {
                    obj.print();
                }
                else
                {
                    std::cout << obj << ' ';
                }
            }
            std::cout << '\n';
        }
        // return;

        std::cout << " cout_receiver: receiver: end" << '\n';
    }

    void set_value() noexcept // NOLINT
    {

        std::cout << " cout_receiver: set_value()" << '\n';
    }

    void set_error(std::exception_ptr err) noexcept // NOLINT
    {
        try
        {
            if (err)
                std::rethrow_exception(err);
            std::cout << "cout_receiver: Empty exception thrown" << '\n';
        }
        // Note: 传统方式：只能枚举，漏了，就丢失信息
        // Note: std::current_exception() 的异常。无需手动设置+枚举
        // catch (const std::bad_exception &e)
        // {
        //     std::cout << "cout_receiver: std::bad_exception thrown: " << e.what() <<
        //     '\n';
        // }
        catch (const std::exception &e)
        {
            // Note: 会打印 std::bad_exception 。 很完美
            std::cout << "cout_receiver: Exception thrown: " << e.what() << '\n';
        }
    }

    void set_stopped() // NOLINT
    {
        // std::terminate();
        std::cout << "cout_receiver: set_stopped() " << '\n';
    }
    auto get_env() const noexcept
    {
        return mcs::execution::empty_env{};
    }
};
