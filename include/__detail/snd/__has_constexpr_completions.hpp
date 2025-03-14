#pragma once

#include "../cmplsigs/__valid_completion_signatures.hpp"

namespace mcs::execution::snd
{

    template <class Sndr, class... Env>
    concept has_constexpr_completions = // exposition only
        cmplsigs::valid_completion_signatures<
            decltype(std::remove_reference_t<Sndr>::template get_completion_signatures<
                     Sndr, Env...>())>;

}; // namespace mcs::execution::snd