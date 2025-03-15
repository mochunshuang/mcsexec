#include <exception>
#include <iostream>

template <const auto &>
struct IN_FUNCTION;

struct IN_FUNCTION2;

template <class... What, class... Info>
[[noreturn, nodiscard]] consteval int invalid_completion_signature(
    Info &&...info); // NOLINT

template <typename T>
consteval void test() // NOLINT
{
    if constexpr (std::is_same_v<T, int>)
        throw std::exception("error msg");
}
/**
 * @brief 没有编译期异常。最佳还是 concept
 *
 * @return int
 */

int main()
{
    // 要求
    // 如何使用 test 组合生成新的类型
    // using T = IN_FUNCTION(test);
    // using T = IN_FUNCTION(&test);
    // using T = IN_FUNCTION2(); //OK

    // using T = IN_FUNCTION2(test);
    // 无法获得 任何的 编译期信息 关于 test的

    std::cout << "main done\n";
    return 0;
}