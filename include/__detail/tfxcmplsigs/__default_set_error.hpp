#pragma once

#include "../__core_types.hpp"
#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::tfxcmplsigs
{

    template <class Err>
    using default_set_error = cmplsigs::completion_signatures<set_error_t(Err)>;

}; // namespace mcs::execution::tfxcmplsigs