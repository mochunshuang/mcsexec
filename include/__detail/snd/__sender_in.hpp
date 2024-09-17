#pragma once
#include "./__get_completion_signatures.hpp"
#include "../cmplsigs/__valid_completion_signatures.hpp"

namespace mcs::execution::snd
{
    template <class Sndr, class Env = ::mcs::execution::empty_env>
    concept sender_in =
        sender<Sndr> && queryable<Env> && requires(Sndr &&sndr, Env &&env) {
            {
                get_completion_signatures(std::forward<Sndr>(sndr),
                                          std::forward<Env>(env))
            } -> cmplsigs::valid_completion_signatures;
        };
}; // namespace mcs::execution::snd