#pragma once

#include "../cmplsigs/__completion_signatures.hpp"
#include "../cmplsigs/__detail/__merge_type_lists.hpp"

namespace mcs::execution::tool
{

    // Note: 模板应该独立放在外面。否则可能使用，但是未定义
    template <typename Fun, typename Sigs>
    struct Generate_V_Sigs;

    template <typename Fun, typename Ts>
    struct Make_Sigs;

    template <typename Tag, typename T>
    struct Sig_result_help;

    template <typename Tag, typename T>
        requires(not std::is_same_v<T, void>)
    struct Sig_result_help<Tag, T>
    {
        using type = cmplsigs::completion_signatures<set_value_t(T)>;
    };

    template <typename Tag>
    struct Sig_result_help<Tag, void>
    {
        using type = cmplsigs::completion_signatures<set_value_t()>;
    };

    template <typename Fun, typename Tag, typename... Ts>
        requires(std::invocable<Fun, Ts...>)
    struct Make_Sigs<Fun, Tag(Ts...)>
    {
        using type =
            typename Sig_result_help<Tag, std::invoke_result_t<Fun, Ts...>>::type;
    };

    template <typename Fun, typename Tag, typename... Ts>
        requires(not std::invocable<Fun, Ts...>)
    struct Make_Sigs<Fun, Tag(Ts...)>
    {
        using type = cmplsigs::completion_signatures<>;
    };

    /**
     * Generate value completion from Fun call with sigs
     */
    template <typename Fun, typename... Sig>
    struct Generate_V_Sigs<Fun, cmplsigs::completion_signatures<Sig...>>
    {

        using type = typename cmplsigs::__detail::merge_type_lists<
            cmplsigs::completion_signatures, typename Make_Sigs<Fun, Sig>::type...>::type;
    };

}; // namespace mcs::execution::tool