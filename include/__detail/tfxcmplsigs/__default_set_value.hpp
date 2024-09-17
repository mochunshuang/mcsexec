#pragma once

#include "../__core_types.hpp"
#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::tfxcmplsigs
{
    template <class... As>
    using default_set_value = cmplsigs::completion_signatures<set_value_t(As...)>;

}; // namespace mcs::execution::tfxcmplsigs