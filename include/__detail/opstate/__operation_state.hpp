#pragma once

#include "./__start.hpp"

#include "../__core_types.hpp"

namespace mcs::execution::opstate
{
    //////////////////////////////////////////////////////////////////////////////
    //  [exec.opstate]
    template <class O>
    concept operation_state =
        std::derived_from<typename O::operation_state_concept, operation_state_t> &&
        std::is_object_v<O> && requires(O &o) {
            { start(o) } noexcept;
        };

} // namespace mcs::execution::opstate