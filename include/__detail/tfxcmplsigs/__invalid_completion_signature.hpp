#pragma once

#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution
{

    struct WITH_FUNCTION;
    struct WITH_SENDER;
    struct WITH_ARGUMENTS;
    struct WITH_ENV;

    struct NOTE_INFO;

    template <const auto &>
    struct IN_ALGORITHM;

    struct IN_TAG;

    // MSG
    struct The_previous_completion_signature_does_not_match_the_current_function;
    struct the_fun_return_type_is_not_a_sndr_in_let_xxx;

    struct dependent_sender_error
    {
    };

    namespace tfxcmplsigs
    {
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

        // template <class... What, class... Info>
        // [[noreturn, nodiscard]] consteval completion_signatures<>
        // invalid_completion_signature(
        //     Info &&...info)
        // {
        //     // TODO(mcs): c++26 才能捕获 consteval 的异常
        //     throw sender_type_check_failure<What...>{std::forward<Info>(info)...};
        // }

    } // namespace tfxcmplsigs

}; // namespace mcs::execution
