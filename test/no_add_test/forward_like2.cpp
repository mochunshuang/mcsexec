#include <tuple>
#include <type_traits>
#include <utility>
#include <string>
#include <variant>

// NOLINTBEGIN
// 从元组中获取元素并按其原始类型转发
template <size_t I, typename... Ts>
decltype(auto) get_and_forward(std::tuple<Ts...> &t)
{
    // 关键：按元组元素的原始类型Ts转发，而非存储的实际类型
    return std::forward_like<std::tuple_element_t<I, std::tuple<Ts...>>>(std::get<I>(t));
}

template <typename T>
auto test_template_type(T &&type)
{
    using T0 = T;
    using T1 = decltype((type));
    using T2 = decltype(type);

    using T3 = decltype((std::forward_like<T>(type)));
    using T4 = decltype((std::forward_like<decltype(type)>(type)));
    using T5 = decltype((std::forward_like<T>(std::forward(type))));
    using T6 = decltype((std::forward_like<decltype(type)>(std::forward(type))));

    using T7 = decltype((std::forward<T>(type)));
    using T8 = decltype((std::forward<decltype(type)>(type)));
    using T9 = decltype((std::forward<T>(std::forward(type))));
    using T10 = decltype((std::forward<decltype(type)>(std::forward(type))));

    struct result_types
    {
        using T0 = T0;
        using T1 = T1;
        using T2 = T2;
        using T3 = T3;
        using T4 = T4;
        using T5 = T5;
        using T6 = T6;
        using T7 = T7;
        using T8 = T8;
        using T9 = T9;
        using T10 = T10;
    };
    return result_types{};
}

template <typename... Ts>
struct VariantForwarder
{
    std::variant<Ts...> var;

    // 按变体中类型Ts的原始值类别转发激活的元素
    template <typename T>
    decltype(auto) forward_as()
    {
        return std::visit(
            [](auto &&val) -> decltype(auto) {
                return std::forward_like<T>(val); // 强制模仿T的类型转发
            },
            var);
    }
};

template <typename T>
struct Wrapper
{
    std::remove_cvref_t<T> storage; // 存储为值类型（去引用和const）

    // 按原始类型T转发（可能是引用或const）
    decltype(auto) forward()
    {
        return std::forward_like<T>(storage);
    }
};

// 测试用例
int main()
{
    // auto && 是万能引用
    {
        std::tuple<int, const double, std::string> t(42, 3.14, "test");
        // 提取第一个元素（声明类型int），即使存储的是int，也按int转发
        auto &&a = get_and_forward<0>(t); // int&（左值引用）
        static_assert(std::is_same_v<decltype(a), int &&>);
        static_assert(std::is_same_v<decltype(get_and_forward<0>(t)), int &&>);

        // 提取第二个元素（声明类型const double），按const double转发
        auto &&b = get_and_forward<1>(t); // const double&（const左值引用）
        static_assert(std::is_same_v<decltype(b), const double &&>);
        static_assert(std::is_same_v<decltype(get_and_forward<1>(t)), const double &&>);

        {
            auto i = 0;
            auto &&a = i;
            static_assert(std::is_same_v<decltype(a), int &>);
            auto &&b = 0;
            static_assert(std::is_same_v<decltype(b), int &&>);

            auto &c = b;
            auto &&d = c;
            static_assert(std::is_same_v<decltype(c), decltype(d)>);

            auto &&e = b; // NOTE: 注意
            static_assert(not std::is_same_v<decltype(e), decltype(b)>);
            static_assert(std::is_same_v<decltype(e), int &>);

            // NOTE:  auto&& 修饰的变量，保底是 引用。
        }
        {
            std::tuple<int, const double, std::string> t(42, 3.14, "test");
            auto &&a = std::get<0>(t);
            auto &&b = std::get<1>(t);

            // NOTE: 少了一个 &
            static_assert(std::is_same_v<decltype(a), int &>);
            static_assert(std::is_same_v<decltype(b), const double &>);

            using T = decltype(t);
            static_assert(
                std::is_same_v<std::tuple<int, const double, std::basic_string<char>>,
                               T>);
        }
    }
    {
        // 使用：即使存储为int，仍能按const int&转发
        Wrapper<const int &> w{42};
        auto &&val = w.forward(); // const int&（而非int&）
        static_assert(std::is_same_v<decltype(val), const int &>);
    }

    return 0;
}
// NOLINTEND