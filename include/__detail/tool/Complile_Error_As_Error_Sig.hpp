#pragma once

#include <exception>

#include "../cmplsigs/__completion_signatures.hpp"
#include "../__functional/__callable.hpp"

namespace mcs::execution::tool
{
    template <typename Fun, typename Completion, typename Fun_Result_Sig>
    struct Complile_Error_As_Error_Sig;

    template <typename Fun, typename Fun_Result_Sig>
    struct Complile_Error_As_Error_Sig<Fun, recv::set_value_t, Fun_Result_Sig>
    {
        static constexpr bool value = false; // NOLINT
    };
    template <typename Fun>
    struct Complile_Error_As_Error_Sig<Fun, recv::set_value_t,
                                       cmplsigs::completion_signatures<>>
    {
        // Note: pre_sig_v_sig + fun not match must Complile_Error
        static constexpr bool value = true; // NOLINT
    };

    template <typename Fun, typename Fun_Result_Sig>
    struct Complile_Error_As_Error_Sig<Fun, recv::set_error_t, Fun_Result_Sig>
    {
        static constexpr bool value = false; // NOLINT
    };

    template <typename Fun>
    struct Complile_Error_As_Error_Sig<Fun, recv::set_error_t,
                                       cmplsigs::completion_signatures<>>
    {
        static constexpr bool value = // NOLINT
            not functional::callable<Fun, decltype(std::current_exception())>;
    };

    template <typename Fun, typename Fun_Result_Sig>
    struct Complile_Error_As_Error_Sig<Fun, recv::set_stopped_t, Fun_Result_Sig>
    {
        static constexpr bool value = false; // NOLINT
    };

    template <typename Fun>
    struct Complile_Error_As_Error_Sig<Fun, recv::set_stopped_t,
                                       cmplsigs::completion_signatures<>>
    {
        static constexpr bool value = // NOLINT
            not functional::callable<Fun>;
    };

}; // namespace mcs::execution::tool