
#include <iostream>

#include "invalid_completion_signature.h"
#include "completion_signatures.h"

struct then_t
{
}; // Tag
constexpr inline then_t then{}; // NOLINT

// NOLINTBEGIN

template <typename Fun, typename... As>
consteval auto get_completion_signatures()
{
    // 简单的判断
    if constexpr (sizeof...(As) == 1)
        return invalid_completion_signature<IN_ALGORITHM<then>, WITH_FUNCTION(Fun),
                                            WITH_ARGUMENTS(As...)>(
            "The function passed to std::execution::then is not callable "
            "with the values sent by the predecessor sender.");
    else
        return completion_signatures<set_value_t(As...)>{};
}

/**
 * @brief 可以抛出任意类型：throw
可以抛出任何类型的对象，包括基本类型、自定义类型、标准库异常类型等。

标准库异常类型：C++ 标准库提供了一组异常类型（如 std::exception、std::runtime_error
等），它们通常继承自 std::exception，并提供了 what() 方法来返回异常信息。
 *
 */
void test() noexcept
{
    try
    {
        // 抛出一个整数
        throw 42;
    }
    catch (int e)
    {
        std::cout << "Caught an integer: " << e << std::endl;
    }
}

// 定义概念：检查 get_completion_signatures 是否合法
template <typename Sndr, typename... Args>
concept ValidCompletionSignatures = requires {
    { get_completion_signatures<Sndr, Args...>() } -> valid_completion_signatures;
};

// NOTE: 用返回 completion_signatures<> 代替抛异常
template <class Sndr, class... Env>
constexpr auto get_completion_signatures_impl()
{
    if constexpr (std::is_same_v<Sndr, int>)
        return completion_signatures<set_value_t(int)>{};
    else
        return completion_signatures<>{};
}

// TODO 使用到了c++26 必定失败
template <class Sndr>
consteval bool is_dependent_sender_helper()
{
    // 是否有异常，用completion_signatures<> 表示异常
    if constexpr (std::is_same_v<decltype(get_completion_signatures_impl<Sndr>()),
                                 completion_signatures<>>)
        return false;
    else
        return true;
}

int main()
{
    {
        struct in_valid;
        static_assert(not valid_completion_signatures<in_valid>);
    }
    // Note: completion_signatures<> 是合法的，completion_signatures实例化的都是合法
    static_assert(valid_completion_signatures<completion_signatures<>>);

    static_assert(valid_completion_signatures<
                  completion_signatures<set_value_t(), set_value_t(int)>>);

    auto fun = [](auto &&...msg) {
    };
    auto c = get_completion_signatures<decltype(fun), int, double>();
    static_assert(
        std::is_same_v<decltype(c), completion_signatures<set_value_t(int, double)>>);

    {
        {
            // NOTE: 编译期异常，目前无法捕获然后解决。 等待c++26
            // auto c = get_completion_signatures<decltype(fun), int>();
            // NOTE: 但是 返回的签名不会有异常. 因为没有调用
            using T = decltype(get_completion_signatures<decltype(fun), int>());
            static_assert(std::is_same_v<T, completion_signatures<>>);
        }

        // 使用概念检查 get_completion_signatures<decltype(fun), int, double>()
        // 是否合法
        static_assert(ValidCompletionSignatures<decltype(fun), int, double>); // 合法

        // 使用概念检查 get_completion_signatures<decltype(fun), int>() 是否合法
        // NOTE: 与异常无关
        static_assert(ValidCompletionSignatures<decltype(fun), int>); // 不合法
#if false
        constexpr auto ill_format = []() -> bool {
            if constexpr (requires {
                              get_completion_signatures<decltype(fun), int, double>();
                          })
                return true;
            else
                return false;
        };
        static_assert(ill_format()); // 合理
        {
            constexpr auto ill_format = []() -> bool {
                if constexpr (requires {
                                  get_completion_signatures<decltype(fun), int, int>();
                              })
                    return true;
                else
                    return false;
            };
            static_assert(not ill_format()); // 编译错误
        }
#endif
    }
    {
        // 模拟操作
        static_assert(not is_dependent_sender_helper<double>());
        // NOTE: 现在符合预期
        // TODO 未来用c++26重写
        static_assert(is_dependent_sender_helper<int>());
    }

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND