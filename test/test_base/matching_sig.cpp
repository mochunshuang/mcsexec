#include <iostream>
#include <type_traits>

template <typename Fun>
struct matching_sig
{
    using type = Fun;
};

template <typename Return, typename... Args>
struct matching_sig<Return(Args...)>
{
    using type = Return(Args &&...);
};

template <typename Fun1, typename Fun2>
inline constexpr bool MATCHING_SIG = // NOLINT
    std::same_as<typename matching_sig<Fun1>::type, typename matching_sig<Fun2>::type>;

void test_function(int a, double b)
{
    std::cout << "a: " << a << ", b: " << b << std::endl;
}

int main()
{
    using original_type = decltype(test_function);
    using transformed_type = typename matching_sig<original_type>::type;
    static_assert(std::is_same_v<original_type, void(int, double)>);
    static_assert(std::is_same_v<transformed_type, void(int &&, double &&)>);

    static_assert(MATCHING_SIG<void(int, double), void(int &&, double &&)>);

    static_assert(MATCHING_SIG<void(int, double), void(int &&, double)>); // 区别
    static_assert(not std::is_same_v<void(int, double), void(int &&, double &)>);

    static_assert(not MATCHING_SIG<void(int, double), void(int &&, double &)>);
    return 0;
}