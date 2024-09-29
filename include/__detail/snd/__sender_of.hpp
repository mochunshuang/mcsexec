#pragma once

#include "./__sender_in_of.hpp"

namespace mcs::execution::snd
{
    template <class Sndr, class... Values>
    concept sender_of = sender_in_of<Sndr, empty_env, Values...>;

}; // namespace mcs::execution::snd