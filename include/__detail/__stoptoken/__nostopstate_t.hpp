#pragma once

namespace mcs::execution::stoptoken
{
    // no-shared-stop-state indicator
    struct nostopstate_t
    {
        explicit nostopstate_t() = default;
    };

}; // namespace mcs::execution::stoptoken