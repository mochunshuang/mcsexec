#pragma once

#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution
{
    struct WITH_FUNCTION;
    struct WITH_SENDER;
    struct WITH_ARGUMENTS;
    struct WITH_ENV;
    struct WITH_SIG;

    struct NOTE_INFO;

    template <const auto &>
    struct IN_ALGORITHM;

    template <const auto &>
    struct IN_FUNCTION;

    struct IN_TAG;
    struct Failed_to_complete_signature_calculation;

    // MSG
    struct The_previous_completion_signature_does_not_match_the_current_function;
    struct the_fun_return_type_is_not_a_sndr_in_let_xxx;
    struct
        the_query_funcation_of_the_env_need_nothrow_and_invokeable_with_the_given_query_type;
    struct cannot_accepts_a_sender_that_set_value_completion_signature_more_than_two;
    struct only_accepts_a_senders_with_one_sig_with_tag_of_set_value_t;
    struct parameter_packageis_of_tag_of_set_value_t_is_void;

    struct not_satisfied_with_the_requirements;

    struct check_completion_signature_error;

    struct require_no_children_sndr;

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

} // namespace mcs::execution