
#include "../../include/execution.hpp"
#include <exception>

using namespace mcs::execution;

template <typename T0, typename T1>
struct is_one_of;

template <typename Sig, typename... T>
struct is_one_of<Sig, completion_signatures<T...>>
{
    static constexpr bool value = (std::same_as<Sig, T> || ...); // NOLINT
};

// Note: T0 or T1 mush unique, no repeat
template <typename T0, typename T1>
struct eq_set_sigs;

template <typename... T0, typename... T1>
    requires(not(sizeof...(T0) == sizeof...(T1)))
struct eq_set_sigs<completion_signatures<T0...>, completion_signatures<T1...>>
{
    static constexpr bool value = false; // NOLINT
};

template <typename... T0, typename... T1>
    requires(sizeof...(T0) == sizeof...(T1))
struct eq_set_sigs<completion_signatures<T0...>, completion_signatures<T1...>>
{
    using Set0 = completion_signatures<T0...>;
    using Set1 = completion_signatures<T1...>;
    static constexpr bool all_in_Set0 = (is_one_of<T1, Set0>::value && ...); // NOLINT
    static constexpr bool all_in_Set1 = (is_one_of<T0, Set1>::value && ...); // NOLINT
    static constexpr bool value = all_in_Set0 && all_in_Set1;                // NOLINT
};

int main()
{

    using V_Sig = recv::set_value_t();
    using E_Sig = recv::set_value_t(std::exception_ptr);
    using S_Sig = recv::set_stopped_t();
    using set_sig0 = cmplsigs::completion_signatures<V_Sig, E_Sig, S_Sig>;
    using set_sig1 = cmplsigs::completion_signatures<V_Sig, S_Sig, E_Sig>;

    // 理论上，set_sig0 和  set_sig1 应该是 集合等价的。如何编译器求值是否相等
    // Note: 集合等价性：如果两个类型列表中的每个类型都在对方列表中，那么它们是等价的

    static_assert(eq_set_sigs<set_sig0, set_sig0>::value);
    static_assert(eq_set_sigs<set_sig0, set_sig1>::value);
    static_assert(eq_set_sigs<set_sig1, set_sig0>::value);

    {
        // Note: undefined behavior . because has repeat sig not set
        using repeat_v = cmplsigs::completion_signatures<V_Sig, V_Sig, E_Sig, S_Sig>;
        using repeat_e = cmplsigs::completion_signatures<V_Sig, S_Sig, S_Sig, E_Sig>;
        static_assert(eq_set_sigs<repeat_v, repeat_e>::value);
    }
    {
        using t0 = cmplsigs::completion_signatures<recv::set_value_t(int &&)>;
        using t1 = cmplsigs::completion_signatures<recv::set_value_t(int)>;
        static_assert(not eq_set_sigs<t0, t1>::value);
    }
    return 0;
}