#include "../test_base_head.hpp"
#include <exception>
#include <tuple>
#include <type_traits>

struct B
{
    template <typename V>
    auto operator()(V &&v) // NOLINT
    {
        if constexpr (std::same_as<std::decay_t<decltype(v)>, int>)
        {
            return std::string("hello");
        }
        else
        {
            return 1;
        }
    }
};

void base();   // NOLINT
void lambda(); // NOLINT

int main()
{

    base();
    // 可以萃取 fun_1 的 参数类型，返回值类型吗？
    return 0;
}

void base()
{
    // 如何表达出 指向 template operator 模板的类型？有这种指向模板的类型吗
    [[maybe_unused]] B b;
    // Note: 无法指向，
    //  using T = &b.template operator();
}

void lambda()
{
    [[maybe_unused]] auto fun = []() {
    };
    auto fun_1 = [](auto &&v) {
        if constexpr (std::same_as<std::decay_t<decltype(v)>, int>)
        {
            return std::string("hello");
        }
        else
        {
            return 1;
        }
    };
    auto fun_2 = [](auto &&...ts) {
        if constexpr (sizeof...(ts) == 0)
        {
            return;
        }
        else if constexpr (sizeof...(ts) == 1)
        {
            return (std::forward<decltype(ts)>(ts), ...);
        }
        else
        {
            return std::make_tuple(std::forward<decltype(ts)>(ts)...);
        }
    };
    // 假设，是否match ，看起来。就只能看，能不能调用了。
    // Note: CS 的逻辑该改了，因为 下一个sndr的函数，可能是模板
    // Note: 如何如何，pre_sndr 已经确定 CS了。遍历 CS，能够完成调用即可
    // 假设: set_value(double),set_value(int)
    using R = decltype(fun_1(std::declval<int>()));
    static_assert(std::is_same_v<R, std::string>);

    {
        using R = decltype(fun_1(std::declval<double>()));
        static_assert(std::is_same_v<R, int>);
    }
    // fun_2
    {
        using R = decltype(fun_2());
        static_assert(std::is_same_v<R, void>);
    }
    {
        using R = decltype(fun_2(std::declval<double>()));
        static_assert(std::is_same_v<R, double>);
    }
    {
        using R = decltype(fun_2(std::declval<double>(), std::declval<double>()));
        static_assert(std::is_same_v<R, std::tuple<double, double>>);
    }
    //
    using Arg0 = std::tuple<int>;
    using Arg1 = std::tuple<>;
    using Arg2 = std::tuple<double>;
    using Arg3 = std::tuple<float>;
    using T [[maybe_unused]] = std::tuple<Arg0, Arg1, Arg2, Arg3>;

    auto func = []<typename... T>(T &&...ts)
        requires(sizeof...(ts) <= 2 &&
                 !(sizeof...(ts) == 1 && (std::is_same_v<std::decay_t<T>, float> || ...)))
    {
        if constexpr (sizeof...(ts) > 0)
            return 0 + (std::forward<decltype(ts)>(ts), ...);
        else
            return;
    };
    // 遍历 T的元素，如果参数，可以调用 func 则记录 fun(ars)的返回值，到 tuple 并返回
    // 如果不能调用，跳过
    // 结果：编译期得到，fun(ars)的返回值组合的tuple
    using Fun = decltype(func);
    {
        using Type = std::invoke_result_t<Fun>;
        static_assert(std::same_as<void, Type>);
        static_assert(std::invocable<Fun>);
        static_assert(std::invocable<Fun, int>);
        static_assert(std::same_as<int, std::invoke_result_t<Fun, int>>);
        static_assert(not std::invocable<Fun, float>);
    }
    // Note: 只需要，有一个 Sig 能够 invoke 就行了。pre_sndr 就能和放弃sndr 连接
    // Note: Next_sndr 怎么写？ 只能用 sigs of pre_sndr + fun of sndr =>all sig
    // Note: 因此无论如何肯定是要 遍历 + result 的
    {
        // 不能有重复
        // using T = std::variant<std::monostate, std::monostate>;
        static_assert(std::is_same_v<decltype(1 + 1.0), double>);
        static_assert(std::is_same_v<decltype(1 + 1.0F), float>);
    }
    {
        // Note: 空返回值，没有体现
        using namespace mcs::execution; // NOLINT
        using Cur = std::conditional_t<std::invocable<Fun, float>, std::tuple<float>,
                                       std::tuple<>>;
        using Pre = std::tuple<int>;

        using All = decltype(std::tuple_cat(std::declval<Cur>(), std::declval<Pre>()));
        static_assert(std::is_same_v<Pre, All>);
    }
}

namespace
{
    // NOLINTNEXTLINE
    auto func = []<typename... T>(T &&...ts)
        requires(sizeof...(ts) <= 2 &&
                 !(sizeof...(ts) == 1 && (std::is_same_v<std::decay_t<T>, float> || ...)))
    {
        if constexpr (sizeof...(ts) > 0)
            return 0 + (std::forward<decltype(ts)>(ts), ...);
        else
            return;
    };

    using Fun = decltype(func);

    using namespace mcs::execution; // NOLINT

    using PRE_Sigs = cmplsigs::completion_signatures<
        recv::set_value_t(), recv::set_value_t(int), recv::set_value_t(float),
        recv::set_value_t(int, double), recv::set_error_t(std::exception_ptr),
        recv::set_stopped_t()>;

    using Error_Sigs =
        cmplsigs::completion_signatures<recv::set_value_t(float),
                                        recv::set_error_t(std::exception_ptr),
                                        recv::set_stopped_t()>;

    // Let_系列
    // Note: 模板应该独立放在外面。否则可能使用，但是未定义
    template <typename Fun, typename Sigs>
    struct Make_Return_Sigs;

    template <typename Fun, typename Ts>
    struct Make_V_Sigs;

    template <typename T>
    struct result_help;

    template <typename T>
        requires(not std::is_same_v<T, void>)
    struct result_help<T>
    {
        using type = cmplsigs::completion_signatures<recv::set_value_t(T)>;
    };

    template <>
    struct result_help<void>
    {
        using type = cmplsigs::completion_signatures<recv::set_value_t()>;
    };

    template <typename Fun, typename... Ts>
        requires(std::invocable<Fun, Ts...>)
    struct Make_V_Sigs<Fun, set_value_t(Ts...)>
    {
        using type = typename result_help<std::invoke_result_t<Fun, Ts...>>::type;
    };

    template <typename Fun, typename... Ts>
        requires(not std::invocable<Fun, Ts...>)
    struct Make_V_Sigs<Fun, set_value_t(Ts...)>
    {
        using type = cmplsigs::completion_signatures<>;
    };

    template <typename Fun, typename... Sig>
    struct Make_Return_Sigs<Fun, cmplsigs::completion_signatures<Sig...>>
    {

        using type = typename cmplsigs::__detail::merge_type_lists<
            cmplsigs::completion_signatures,
            typename Make_V_Sigs<Fun, Sig>::type...>::type;
    };

    // TEST
    using Filter_V_Sig =
        cmplsigs::__detail::filter_sigs_by_completion<set_value_t, PRE_Sigs>::type;
    using Ret = Make_Return_Sigs<Fun, Filter_V_Sig>::type;

    static_assert(std::is_same_v<Ret, tool::Generate_V_Sigs<Fun, Filter_V_Sig>::type>);

    static_assert(std::is_same_v<Ret,
                                 cmplsigs::completion_signatures<
                                     recv::set_value_t(), // 返回值为空
                                     recv::set_value_t(int), recv::set_value_t(double)>>);

    using Filter_V_Sig_1 =
        cmplsigs::__detail::filter_sigs_by_completion<set_value_t, Error_Sigs>::type;
    using R_1 = Make_Return_Sigs<Fun, Filter_V_Sig_1>::type;
    // Note: 说明没有一个 Sig 能够调用 Fun
    static_assert(std::is_same_v<R_1, cmplsigs::completion_signatures<>>);
    static_assert(std::is_same_v<R_1, tool::Generate_V_Sigs<Fun, Filter_V_Sig_1>::type>);

} // namespace