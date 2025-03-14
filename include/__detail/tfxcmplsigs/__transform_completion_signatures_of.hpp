#pragma once

#include "./__transform_completion_signatures.hpp"
#include "../snd/__sender_in.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

namespace mcs::execution::tfxcmplsigs
{
    template <snd::sender Sndr, class... Env>
        requires snd::sender_in<Sndr, Env...>
    using transform_completion_signatures_of =
        decltype(transform_completion_signatures<
                 snd::completion_signatures_of_t<Sndr, Env...>>());

}; // namespace mcs::execution::tfxcmplsigs