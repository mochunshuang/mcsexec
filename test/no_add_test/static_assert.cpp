#include <type_traits>
// NOLINTBEGIN
template <typename T>
void check_type()
{
    if constexpr (std::is_integral_v<T>)
    {
        static_assert(std::is_same_v<T, int>, "T must be int");
    }
    else
    {
        static_assert(std::is_same_v<T, double>, "T must be double");
    }
}

template <typename T>
concept check = std::is_same_v<T, int> || std::is_same_v<T, double>;

template <check T>
void check_type2();

template <typename T>
void check_type3()
{
    static_assert(std::disjunction_v<std::is_same<T, int>, std::is_same<T, double>>,
                  "T must be int or double");
    if constexpr (std::is_integral_v<T>)
    {
        static_assert(std::is_same_v<T, int>, "T must be int");
    }
    else
    {
        static_assert(std::is_same_v<T, double>, "T must be double");
    }
}
struct INVALID_Number_of_Fun_Parameters
{
};
template <typename T>
consteval void check_type4()
{
    if constexpr (not check<T>)
        throw INVALID_Number_of_Fun_Parameters{};
}
consteval void test(auto t)
{
    check_type4<std::decay_t<decltype(t)>>();
}

consteval void test2(auto t)
{
    test(t);
}

static constexpr auto test3 = [](auto t) {
    test2(t);
};

void test4(auto t)
{
    test3(t);
}

int main()
{
    check_type<int>();    // 正常编译，因为 T 是 int，满足 std::is_integral_v<T>
    check_type<double>(); // 正常编译，因为 T 是 double，不满足
                          // std::is_integral_v<T>，所以 else 分支中的 static_assert
                          // 被裁剪
    //  如何让这里爆红呢？
    // check_type<float>(); // 编译错误，但是错误在 check_type()内部代码爆红
    // std::is_integral_v<T>，也不满足 std::is_same_v<T, double>

    // check_type2<float>(); // 这里爆红，合格

    // check_type3<float>(); // 不在 这里爆红，不合格

    test(1.0);
    // test(1.0f); // 合格

    // test2(1.0f); // 报错，可以

    test3(1); // 编译期不能异常，这个是不会变的
    // test4(1); // NOTE: 不写，还得必须catch，  + 概念才是最好的方式
}
// NOLINTEND