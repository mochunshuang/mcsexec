#pragma once
#include "../cmplsigs/__completion_signatures.hpp"
#include "./__err_msg.hpp"

namespace mcs::execution
{
    namespace diagnostics
    {
        template <int v>
        concept handle_error_completion_fun_param_count = (v == 1);
        template <int v>
        concept handle_stopped_completion_fun_param_count = (v == 0);

        // NOLINTBEGIN
        template <class... What>
        struct sender_type_check_failure
        {
            template <class... Info>
            consteval explicit sender_type_check_failure(Info &&.../*unused*/)
            {
            }
        };
        // NO defined replace throw
        template <class... What, class... Info>
        [[noreturn, nodiscard]] consteval cmplsigs::completion_signatures<>
        invalid_completion_signature(Info &&...info);
    }; // namespace diagnostics

}; // namespace mcs::execution