#include <tuple>
#include <iostream>
#include <functional>
#include <string>

// 定义 product_type
template <typename... Ts>
struct product_type
{
    std::tuple<Ts...> elements;

    product_type(Ts &&...args) : elements(std::forward<Ts>(args)...) {}

    template <std::size_t I>
    [[nodiscard]] auto get() const
    {
        return std::get<I>(elements);
    }
};

// 推导指引
template <typename... T>
product_type(T &&...) -> product_type<std::decay_t<T>...>;

// 特化 std::tuple_size
namespace std
{
    template <typename... Ts>
    struct tuple_size<product_type<Ts...>>
        : std::integral_constant<std::size_t, sizeof...(Ts)>
    {
    };

    // 特化 const 版本的 std::tuple_size
    template <typename... Ts>
    struct tuple_size<const product_type<Ts...>>
        : std::integral_constant<std::size_t, sizeof...(Ts)>
    {
    };

    // 特化引用版本的 std::tuple_size
    template <typename... Ts>
    struct tuple_size<product_type<Ts...> &>
        : std::integral_constant<std::size_t, sizeof...(Ts)>
    {
    };

    // 特化 const 引用版本的 std::tuple_size
    template <typename... Ts>
    struct tuple_size<const product_type<Ts...> &>
        : std::integral_constant<std::size_t, sizeof...(Ts)>
    {
    };
} // namespace std

// 特化 std::tuple_element
namespace std
{
    template <std::size_t I, typename... Ts>
    struct tuple_element<I, product_type<Ts...>> : tuple_element<I, std::tuple<Ts...>>
    {
    };

    // 特化 const 版本的 std::tuple_element
    template <std::size_t I, typename... Ts>
    struct tuple_element<I, const product_type<Ts...>>
        : tuple_element<I, const std::tuple<Ts...>>
    {
    };

    // 特化引用版本的 std::tuple_element
    template <std::size_t I, typename... Ts>
    struct tuple_element<I, product_type<Ts...> &> : tuple_element<I, std::tuple<Ts...> &>
    {
    };

    // 特化 const 引用版本的 std::tuple_element
    template <std::size_t I, typename... Ts>
    struct tuple_element<I, const product_type<Ts...> &>
        : tuple_element<I, const std::tuple<Ts...> &>
    {
    };
} // namespace std

// 特化 std::get<I>
namespace std
{
    template <std::size_t I, typename... Ts>
    auto get(const product_type<Ts...> &pt)
    {
        return pt.template get<I>();
    }

    // 特化非 const 版本的 std::get<I>
    template <std::size_t I, typename... Ts>
    auto get(product_type<Ts...> &pt)
    {
        return pt.template get<I>();
    }

}; // namespace std

namespace std
{
    template <typename... _Elements>
    inline constexpr bool __is_tuple_like_v<::product_type<_Elements...>> = true;
}

// 测试函数
void test5_invoke_and_apply()
{
    // 创建一个 product_type 实例
    auto pt = product_type{42, 3.14, "Hello, World!"};

    // 定义一个函数，接受三个参数
    auto print_values = [](int i, double d, const std::string &s) {
        std::cout << "int: " << i << ", double: " << d << ", string: " << s << std::endl;
    };

    // 使用 std::invoke 调用函数，传递 product_type 的元素
    std::invoke(print_values, std::get<0>(pt), std::get<1>(pt), std::get<2>(pt));

    // 使用 std::apply 调用函数，传递 product_type 的元素
    std::apply(print_values, pt); // 现在可以正常工作
}

int main()
{
    // Note: template< class T >
    //  concept tuple - like = /* 见下文 */;
    // (1)(C++ 23 起)(仅用于阐述 *)
    test5_invoke_and_apply();
    std::cout << "hello world\n";
    return 0;
}