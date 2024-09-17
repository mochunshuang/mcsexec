#pragma once

#include "../../general/__impls_for.hpp"
#include "../../../__functional/__call_result_t.hpp"
#include "../../__tag_of_t.hpp"

namespace mcs::execution::snd::__detail::mate_type
{
    template <class Sndr, class Rcvr> // exposition only
    using state_type = std::decay_t<functional::call_result_t<
        decltype(general::impls_for<snd::tag_of_t<Sndr>>::get_state), Sndr, Rcvr &>>;

}; // namespace mcs::execution::snd::__detail::mate_type