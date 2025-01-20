#pragma once

#include "./__finite_inplace_stop_token.hpp"

namespace mcs::execution::stoptoken
{
    // [stoptoken.single], class single_inplace_stop_token
    using single_inplace_stop_token = finite_inplace_stop_token<1, 0>;

}; // namespace mcs::execution::stoptoken