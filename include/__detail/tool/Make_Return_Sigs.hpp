#pragma once

#include "../cmplsigs/__completion_signatures.hpp"
#include "../cmplsigs/__detail/__merge_type_lists.hpp"

namespace mcs::execution::tool
{

    template <typename Fun, typename Sigs>
    struct Make_Return_Sigs;

    template <typename Fun, typename... Sig>
    struct Make_Return_Sigs<Fun, cmplsigs::completion_signatures<Sig...>>
    {
        template <typename T>
        struct result_help
        {
            using type = cmplsigs::completion_signatures<recv::set_value_t(T)>;
        };
        template <>
        struct result_help<void>
        {
            using type = cmplsigs::completion_signatures<recv::set_value_t()>;
        };

        template <typename Ts>
        struct Make_V_Sigs;
        template <typename... Ts>
            requires(std::invocable<Fun, Ts...>)
        struct Make_V_Sigs<set_value_t(Ts...)>
        {
            using type = result_help<std::invoke_result_t<Fun, Ts...>>::type;
        };
        template <typename... Ts>
            requires(not std::invocable<Fun, Ts...>)
        struct Make_V_Sigs<set_value_t(Ts...)>
        {
            using type = cmplsigs::completion_signatures<>;
        };

        using type = typename cmplsigs::__detail::merge_type_lists<
            cmplsigs::completion_signatures, typename Make_V_Sigs<Sig>::type...>::type;
    };

}; // namespace mcs::execution::tool