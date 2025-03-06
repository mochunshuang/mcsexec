#include <exception>
#include <iostream>

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
    test<double>();
    // test<int>();
    std::cout << "main done\n";
    return 0;
}