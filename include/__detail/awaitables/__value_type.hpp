#pragma once
#include "../queries/__env_of_t.hpp"
#include "../cmplsigs/__single_sender_value_type.hpp"

namespace mcs::execution::awaitables
{
    template <class Sndr, class Promise>
    using value_type =
        cmplsigs::single_sender_value_type<Sndr, queries::env_of_t<Promise>>;
};