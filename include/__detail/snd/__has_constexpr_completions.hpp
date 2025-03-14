#pragma once

#include "../cmplsigs/__valid_completion_signatures.hpp"

namespace mcs::execution::snd
{

    template <class Sndr, class... Env>
    concept has_constexpr_get_completion_signatures = // exposition only
        cmplsigs::valid_completion_signatures<
            decltype(std::remove_reference_t<Sndr>::template get_completion_signatures<
                     Sndr, Env...>())>;
    // completion_signatures
    template <class Sndr, class... Env>
    concept has_completion_signatures_type = // exposition only
        cmplsigs::valid_completion_signatures<typename Sndr::completion_signatures>;

    template <class Sndr, class... Env>
    concept has_constexpr_completions = // exposition only
        has_constexpr_get_completion_signatures<Sndr, Env...> ||
        has_completion_signatures_type<Sndr, Env...>;

}; // namespace mcs::execution::snd