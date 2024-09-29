#pragma once

#include "./__sender_in.hpp"
#include "./general/__MATCHING_SIG.hpp"

#include "../cmplsigs/__value_types_of_t.hpp"

namespace mcs::execution::snd
{
    template <class... As>
    using value_signature = set_value_t(As...); // exposition only

    template <class Sndr, class Env, class... Values>
    concept sender_in_of =
        sender_in<Sndr, Env> &&
        general::MATCHING_SIG< // see [exec.general]
            set_value_t(Values...),
            cmplsigs::value_types_of_t<Sndr, Env, value_signature, std::type_identity_t>>;

}; // namespace mcs::execution::snd