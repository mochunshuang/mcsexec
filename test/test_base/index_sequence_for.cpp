#include <iostream>
#include <string>
#include <utility>

template <::std::size_t, typename T>
struct product_type_element
{
    T value; // NOLINT
};

template <typename, typename...>
struct product_type_base;

template <::std::size_t... I, typename... T>
struct product_type_base<::std::index_sequence<I...>, T...>
    : product_type_element<I, T>...
{
};

template <typename... T>
struct product_type : product_type_base<::std::index_sequence_for<T...>, T...>
{
};

// 推导指南
template <typename... T>
product_type(T &&...) -> product_type<::std::decay_t<T>...>;

int main()
{
    // 显式指定模板参数类型
    product_type<int, std::string> pt{42, "Hello"};

    // 访问元素
    std::cout << "int value: " << static_cast<product_type_element<0, int> &>(pt).value
              << std::endl;
    std::cout << "string value: "
              << static_cast<product_type_element<1, std::string> &>(pt).value
              << std::endl;

    return 0;
}