#pragma once

#include "./__completion_signatures.hpp"

namespace mcs::execution::cmplsigs
{
    namespace __detail
    {
        template <class Sigs>
        inline constexpr bool __completion_signatures = false; // NOLINT

        template <completion_signature... Sigs>
        inline constexpr bool // NOLINTNEXTLINE
            __completion_signatures<cmplsigs::completion_signatures<Sigs...>> = true;
    }; // namespace __detail

    template <class Completions>
    concept valid_completion_signatures = __detail::__completion_signatures<Completions>;

}; // namespace mcs::execution::cmplsigs
