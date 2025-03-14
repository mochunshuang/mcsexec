#include <iostream>
#include <type_traits>
// NOLINTBEGIN
// 定义 set_value_t 和 Shape
struct set_value_t
{
};
struct Shape
{
};

// 定义两个函数，一个 noexcept，一个非 noexcept
void fun_noexcept(Shape, int &) noexcept {}

void fun_noexcept2(Shape, Shape &) noexcept {}

// 测试引用折叠的 lambda
constexpr auto test_reference_collapsing = []<class Tag, class... As>(Tag (*)(As...)) {
    if constexpr (std::same_as<Tag, set_value_t>)
    {
        if constexpr (not std::invocable<decltype(fun_noexcept), Shape, As &...>)
        {
            return throw;
        }
        else
        {
            return std::is_nothrow_invocable_v<decltype(fun_noexcept), Shape, As &...>;
        }
    }
    else
    {
        return true;
    }
};

constexpr auto test_reference_collapsing2 = []<class Tag, class... As>(Tag (*)(As...)) {
    if constexpr (std::same_as<Tag, set_value_t>)
    {
        if constexpr (not std::invocable<decltype(fun_noexcept2), Shape, As &...>)
        {
            return throw;
        }
        else
        {
            return std::is_nothrow_invocable_v<decltype(fun_noexcept2), Shape, As &...>;
        }
    }
    else
    {
        return true;
    }
};

int main()
{
    // 测试引用折叠
    using Sig1 = set_value_t(int &);  // int& 作为参数
    using Sig2 = set_value_t(int &&); // int&& 作为参数
    using Sig3 = set_value_t(int);

    // 测试 int&
    constexpr bool result1 = test_reference_collapsing(static_cast<Sig1 *>(nullptr));
    std::cout << "Result for int&: " << result1 << std::endl; // 应为 true
    static_assert(result1);

    // 测试 int&&
    constexpr bool result2 = test_reference_collapsing(static_cast<Sig2 *>(nullptr));
    std::cout << "Result for int&&: " << result2 << std::endl; // 应为 true
    static_assert(result2);

    constexpr bool result3 = test_reference_collapsing(static_cast<Sig3 *>(nullptr));
    static_assert(result3);

    {
        using Sig1 = set_value_t(Shape &);  // int& 作为参数
        using Sig2 = set_value_t(Shape &&); // int&& 作为参数
        using Sig3 = set_value_t(Shape);
        static_assert(test_reference_collapsing2(static_cast<Sig1 *>(nullptr)));
        static_assert(test_reference_collapsing2(static_cast<Sig2 *>(nullptr)));
        static_assert(test_reference_collapsing2(static_cast<Sig3 *>(nullptr)));
    }

    // NOTE: c++的引用折叠，大大简化了。元编程的心智负担
    return 0;
}
// NOLINTEND