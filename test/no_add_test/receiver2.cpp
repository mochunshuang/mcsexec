#include <iostream>
#include <tuple>
#include <variant>

// NOLINTBEGIN
struct receiver
{
    template <class... Args>
    void set_value(Args &&...args) noexcept
    {
        ((std::cout << args << ","), ...);
        std::cout << '\n';
    }
};

struct receiver2
{
    template <class... Args>
    void set_value(std::variant<std::tuple<Args...>> t) noexcept
    {
        // 从variant中提取tuple并使用std::apply解包参数
        std::apply(
            [this](auto &&...args) {
                recv.set_value(std::forward<decltype(args)>(args)...);
            },
            std::get<std::tuple<Args...>>(t));
    }

    receiver recv;
};

int main()
{
    receiver2 recv2;
    // 示例：传递包含tuple的variant
    recv2.set_value(
        std::variant<std::tuple<int, double>>{std::in_place_index<0>, 42, 3.14});
    return 0;
}
// NOLINTEND