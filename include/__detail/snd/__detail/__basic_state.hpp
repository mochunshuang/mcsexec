#pragma once

#include "./mate_type/__state_type.hpp"
#include "../../__functional/__nothrow_callable.hpp"

namespace mcs::execution::snd::__detail
{
    template <class Sndr, class Rcvr>
    struct basic_state // exposition only
    {
        basic_state(Sndr &&sndr, Rcvr &&rcvr) noexcept(
            std::is_nothrow_move_constructible_v<Rcvr> &&
            functional::nothrow_callable<
                decltype(general::impls_for<tag_of_t<Sndr>>::get_state), Sndr, Rcvr &>)
            : rcvr(std::move(rcvr)), state(general::impls_for<tag_of_t<Sndr>>::get_state(
                                         std::forward<Sndr>(sndr), this->rcvr))
        {
        }

        Rcvr rcvr;                               // exposition only // NOLINT
        mate_type::state_type<Sndr, Rcvr> state; // exposition only // NOLINT
    };
}; // namespace mcs::execution::snd::__detail