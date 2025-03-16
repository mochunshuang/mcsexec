#include <iostream>
#include <tuple>
#include <type_traits>

// NOLINTBEGIN
struct default_impls
{
    template <class Sndr, class... Env>
    static constexpr bool checktype();
};

struct A
{
    int value{};
};
struct B
{
    int value{};
};

template <class Tag>
struct impls_for : default_impls
{
};

// 特化需要保持接口一致
template <>
struct impls_for<A> : default_impls
{
    template <class Sndr>
    static constexpr bool checktype()
    {
        return true;
    }
};

template <>
struct impls_for<B> : default_impls
{
    template <class Sndr>
    static constexpr bool checktype()
    {
        return false;
    }
};

// 修正模板参数名
template <class Sndr, class... Env>
consteval bool compute_value()
{
    return impls_for<Sndr>::template checktype<Sndr, Env...>();
}

// 修正concept模板参数
template <class Sndr, class... Env>
concept check_type = impls_for<Sndr>::template checktype<Sndr, Env...>();

template <class Sndr, class... Env>
concept check_type2 = compute_value<Sndr, Env...>();

// 修正参数包展开语法
template <typename T, typename... Env>
    requires(check_type<T, Env...> && check_type2<T, Env...>)
void print_value(T value, Env &&...)
{
    std::cout << "Value: " << value.value << '\n';
}

void test(auto v)
{
    print_value(v);
}

template <typename Tag, typename Child>
struct BaseSndr
{
    using Child_type = Child;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures()
    {
        return impls_for<Tag>::template checktype<Self, Env...>();
    }
};

template <typename Tag, class Sndr, class... Env>
consteval bool is_check_pass_impl()
{
    return impls_for<Tag>::template checktype<Sndr, Env...>();
}

template <auto>
concept is_constant = true;

template <typename Tag, class Sndr, class... Env>
concept is_check_pass = is_check_pass_impl<Tag, Sndr, Env...>();

template <class Sndr, class... Env>
concept sender_in = is_constant<Sndr::template get_completion_signatures<Sndr, Env...>()>;

template <typename Tag, typename Data>
    requires(
        sender_in<BaseSndr<Tag, Data>> &&
        is_check_pass<std::decay_t<Tag>, BaseSndr<std::decay_t<Tag>, std::decay_t<Data>>>)
constexpr auto make_sender(Tag &&, Data &&)
{
    return BaseSndr<Tag, Data>{};
}

struct just_t
{
    template <class Ts>
    constexpr auto operator()(Ts &&ts) const noexcept
    {
        return make_sender(*this, std::forward<Ts>(ts));
    }
};
template <>
struct impls_for<just_t> : default_impls
{
    template <class Sndr, class... Env>
    static constexpr bool checktype()
    {
        if constexpr (std::is_same_v<Sndr, BaseSndr<just_t, A>>)
            return true;
        return false;
    }
};

constexpr inline just_t just{};

// 概念放在哪里，哪里才判断
struct just_t2
{
    template <class Ts>
        requires(is_check_pass<std::decay_t<just_t2>,
                               BaseSndr<std::decay_t<just_t2>, std::decay_t<Ts>>>)
    constexpr auto operator()(Ts &&ts) const noexcept
    {
        return make_sender(*this, std::forward<Ts>(ts));
    }
};

constexpr inline just_t2 just2{};

template <>
struct impls_for<just_t2> : default_impls
{
    template <class Sndr, class... Env>
    static constexpr bool checktype()
    {
        if constexpr (std::is_same_v<Sndr, BaseSndr<just_t2, A>>)
            return true;
        return false;
    }
};

struct just_t3
{
    template <class Ts>
        requires(is_check_pass<std::decay_t<just_t3>,
                               BaseSndr<std::decay_t<just_t3>, std::decay_t<Ts>>>)
    constexpr auto operator()(Ts &&ts) const noexcept
    {
        return make_sender(*this, std::forward<Ts>(ts));
    }
};

constexpr inline just_t3 just3{};
struct WITH_FUNCTION;
struct WITH_SENDER;
struct WITH_ARGUMENTS;
struct WITH_ENV;
struct WITH_SIG;
template <class... What, class... Info>
[[noreturn, nodiscard]] consteval bool check_fails(Info &&...info);

template <>               // NOTE: 可以不用继承，只要你能确定你用到的，都都定义了
struct impls_for<just_t3> // : default_impls
{
    template <class Sndr> // NOTE: 模板之间没有任何关系
    static constexpr bool checktype()
    {
        if constexpr (std::is_same_v<Sndr, BaseSndr<just_t3, A>>)
            return true;
        else
        {
            // struct INVOID
            // {
            // };
            // return (throw INVOID{}, false); // 比不上直接false，概念提示更有意思

            check_fails<WITH_ARGUMENTS(A), WITH_SENDER(Sndr)>();
            return false;
        }
    }
};

int main()
{
    print_value(A{}); // 正确调用方式
    // print_value(B{});    // 编译失败（约束不满足）

    {
        test(A{});
        // test(B{}); // 不在这里爆红
    }
    {
        just(A{}); // OK

        // just(B{}); // 不爆红，不好 //NOTE: 解决不了哦
    }
    {
        just2(A{});
        // just2(B{}); // 直接爆红，最好是这种信息
    }
    {
        just3(A{});
        // Note: 目前不够好。 但是配合 check_fails 还不错。 通过未定义实现编译期错误检查
        // just3(B{}); // 直接爆红，最好是这种信息
    }
}
// NOLINTEND