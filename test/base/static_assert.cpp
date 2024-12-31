#include <type_traits>
template <typename T>
void check_type() // NOLINT
{
    if constexpr (std::is_integral_v<T>)
    {
        static_assert(sizeof(T) <= 4, "Integral type is too large!");
    }
    else
    {
        static_assert(std::is_floating_point_v<T>,
                      "Type must be integral or floating point!");
    }
}

int main()
{
    check_type<int>();    // 正常编译
    check_type<double>(); // 正常编译
    // check_type<char*>(); // 编译错误：Type must be integral or floating point!

    // Note: if constexpr 不会影响 static_assert的计算
#if 0
    if constexpr (true)
    {
        static_assert(true, "Integral type is too large!");
    }
    else
    {
        static_assert(false, "Type must be integral or floating point!");
    }
#endif
    // 可以skip
    static_assert(true || false, "skip "); // NOLINT
    return 0;
}