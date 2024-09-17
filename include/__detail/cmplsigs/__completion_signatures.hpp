#pragma once

#include "./__completion_signature.hpp"

namespace mcs::execution::cmplsigs
{
    template <completion_signature...>
    struct completion_signatures
    {
    };
}; // namespace mcs::execution::utils