#include <string>
#include <type_traits>
#include <iostream>

// 定义 set_value_t 类型
struct set_value_t;

// 自定义谓词：判断类型是否是 set_value_t
template <typename T>
struct is_set_value_t_predicate
{
    static constexpr bool value = std::is_same_v<set_value_t, T>; // NOLINT
};
template <typename T>
struct is_not_set_value_t_predicate
{
    static constexpr bool value = !std::is_same_v<set_value_t, T>; // NOLINT
};

template <typename T>
struct is_string
{
    static constexpr bool value = std::is_same_v<std::string, T>; // NOLINT
};

template <template <typename...> class Template, template <typename> class Predicate,
          typename Type>
struct Select_Type;

// 特化：处理模板实例
template <template <typename...> class Template, template <typename> class Predicate,
          typename... T>
struct Select_Type<Template, Predicate, Template<T...>>
{
    template <typename Rest, typename Collect>
    struct Select;

    template <typename... Added>
    struct Select<Template<>, Template<Added...>>
    {
        using type = Template<Added...>;
    };

    template <typename Cur, typename... Rest, typename... Added>
    struct Select<Template<Cur, Rest...>, Template<Added...>>
    {
        using type = std::conditional_t<
            Predicate<Cur>::value,
            typename Select<Template<Rest...>, Template<Added..., Cur>>::type,
            typename Select<Template<Rest...>, Template<Added...>>::type>;
    };

    using type = typename Select<Template<T...>, Template<>>::type;
};

// 定义 MyTemplate
template <typename... T>
struct MyTemplate
{
};

int main()
{
    using T = MyTemplate<set_value_t, int, set_value_t, double, set_value_t, char>;
    // 测试用例 1：保留 set_value_t
    using result1 = Select_Type<MyTemplate, is_set_value_t_predicate, T>::type;
    static_assert(
        std::is_same_v<MyTemplate<set_value_t, set_value_t, set_value_t>, result1>,
        "Test 1 failed!");

    // 测试用例 2：跳过 set_value_t（通过调整 Predicate 的行为）
    using result2 = Select_Type<MyTemplate, is_not_set_value_t_predicate, T>::type;
    static_assert(std::is_same_v<MyTemplate<int, double, char>, result2>,
                  "Test 2 failed!");

    using result3 = Select_Type<MyTemplate, is_string, T>::type;
    static_assert(std::is_same_v<result3, MyTemplate<>>, "Test 3 failed!");

    std::cout << "All tests passed!" << '\n';
    return 0;
}