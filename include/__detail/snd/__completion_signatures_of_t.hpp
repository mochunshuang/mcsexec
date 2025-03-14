#pragma once

#include "./__get_completion_signatures.hpp"
#include "./__sender_in.hpp"

namespace mcs::execution::snd
{

    template <class Sndr, class... Env>
        requires snd::sender_in<Sndr, Env...>
    using completion_signatures_of_t =
        decltype(get_completion_signatures<Sndr, Env...>());

}; // namespace mcs::execution::snd
