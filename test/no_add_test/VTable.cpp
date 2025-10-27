#include <iostream>
#include <string>
#include <utility>

// NOLINTBEGIN

// 主模板 - auto返回类型
template <typename T>
auto process_template(T value)
{
    std::cout << "Processing: " << value << std::endl;
    return value;
}

// 这个特化是无效的！因为主模板使用auto返回类型
// 编译器无法知道你想要特化成什么返回类型
// template <>
// std::string process_template<float>(float value)  // ❌ 编译错误
// {
//     return "123";
// }

struct test
{
    static constexpr auto chan = [](auto input) {
        using InputType = std::decay_t<decltype(input)>;
        // 这里只能调用主模板，无法自动选择不同的返回类型
        return process_template(input); // 总是返回InputType
    };

    auto invoke(auto v)
    {
        return call_sequence(v);
    }

    decltype(chan) call_sequence = chan;
};

struct scheduler_0
{
    struct sender_0
    {
    };
    auto schedule()
    {
        return sender_0{};
    }
};
struct scheduler_1
{
    struct sender_1
    {
    };
    auto schedule()
    {
        return sender_1{};
    }
};

template <typename T>
constexpr auto get_schedule_type = [] {
    if constexpr (requires { std::declval<T &>().schedule(); })
    {
        return std::type_identity<decltype(std::declval<T &>().schedule())>{};
    }
    else
    {
        return std::type_identity<void>{};
    }
}();

int main()
{
    test t;

    // 这些都能正常工作，但返回类型总是与输入类型相同
    auto r1 = t.invoke(1);    // int -> int
    auto r2 = t.invoke(1.0);  // double -> double
    auto r3 = t.invoke(1.0f); // float -> float ❌ 无法返回string

    static_assert(std::is_same_v<decltype(r1), int>);
    static_assert(std::is_same_v<decltype(r2), double>);
    static_assert(std::is_same_v<decltype(r3), float>); // 不是std::string!

    std::cout << "Types: " << typeid(r1).name() << ", " << typeid(r2).name() << ", "
              << typeid(r3).name() << std::endl;

    using ScheduleType0 = typename decltype(get_schedule_type<scheduler_0>)::type;
    using ScheduleType1 = typename decltype(get_schedule_type<scheduler_1>)::type;

    static_assert(std::is_same_v<ScheduleType0, scheduler_0::sender_0>);
    static_assert(std::is_same_v<ScheduleType1, scheduler_1::sender_1>);

    return 0;
}
// NOLINTEND