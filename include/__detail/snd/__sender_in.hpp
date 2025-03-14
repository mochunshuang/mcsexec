#pragma once
#include "./__get_completion_signatures.hpp"
#include "./__is_constant.hpp"

namespace mcs::execution::snd
{
    template <class Sndr, class... Env>
    concept sender_in = sender<Sndr> && (queryable<Env> && ...) &&
                        is_constant<get_completion_signatures<Sndr, Env...>()>;

}; // namespace mcs::execution::snd