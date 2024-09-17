#pragma once

#include "./__state_type.hpp"

namespace mcs::execution::snd::__detail::mate_type
{
    template <class Index, class Sndr, class Rcvr> // exposition only
    using env_type =
        functional::call_result_t<decltype(general::impls_for<tag_of_t<Sndr>>::get_env),
                                  Index, state_type<Sndr, Rcvr> &, const Rcvr &>;

}; // namespace mcs::execution::snd::__detail::mate_type
