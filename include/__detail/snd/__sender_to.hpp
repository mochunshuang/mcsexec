#pragma once

#include "./__sender_in.hpp"
#include "./__completion_signatures_of_t.hpp"
#include "../conn/__connect.hpp"

#include "../recv/__receiver_of.hpp"
#include "../queries/__env_of_t.hpp"

namespace mcs::execution::snd
{

    template <class Sndr, class Rcvr>
    concept sender_to =
        sender_in<Sndr, queries::env_of_t<Rcvr>> &&
        recv::receiver_of<
            Rcvr, snd::completion_signatures_of_t<Sndr, queries::env_of_t<Rcvr>>> &&
        requires(Sndr &&sndr, Rcvr &&rcvr) {
            conn::connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr));
        };

}; // namespace mcs::execution::snd