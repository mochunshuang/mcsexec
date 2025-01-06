#pragma once

#include "./__forwarding_query.hpp"

namespace mcs::execution::queries
{
    template <class T>
    concept forwardingquery = forwarding_query(T{}); // exposition only
};