#pragma once

#include "../cmplsigs/__single_sender_value_type.hpp"

namespace mcs::execution::snd
{

    template <class Sndr, class Env>
    concept single_sender = sender_in<Sndr, Env> && requires {
        typename cmplsigs::single_sender_value_type<Sndr, Env>;
    };

}; // namespace mcs::execution::snd