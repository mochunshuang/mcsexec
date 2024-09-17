#pragma once

#include "../__core_types.hpp"

namespace mcs::execution::cmplsigs
{
    namespace __detail
    {
        template <class _Sig>
        inline constexpr bool __is_compl_sig = false; // NOLINT

        template <class... _Args>
        inline constexpr bool __is_compl_sig<set_value_t(_Args...)> = true; // NOLINT
        template <class _Error>
        inline constexpr bool __is_compl_sig<set_error_t(_Error)> = true; // NOLINT
        template <>
        inline constexpr bool __is_compl_sig<set_stopped_t()> = true; // NOLINT

        template <class Fn>
        concept completion_signature = __is_compl_sig<Fn>;

    }; // namespace __detail

    ////////////////////////////////////
    // [exec.utils.cmplsigs]
    template <class Fn>
    concept completion_signature = __detail::completion_signature<Fn>;
    
}; // namespace mcs::execution::cmplsigs