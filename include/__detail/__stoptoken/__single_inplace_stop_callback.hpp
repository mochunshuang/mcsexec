#pragma once

#include "./__finite_inplace_stop_callback.hpp"

namespace mcs::execution::stoptoken
{
    // [stopcallback.single], class template single_inplace_stop_callback
    template <__detail::invocable_destructible CB>
    using single_inplace_stop_callback = finite_inplace_stop_callback<1, 0, CB>;

}; // namespace mcs::execution::stoptoken