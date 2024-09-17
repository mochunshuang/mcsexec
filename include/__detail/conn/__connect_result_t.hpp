#pragma once

#include <utility>
#include "./__connect.hpp"

namespace mcs::execution::conn
{
    template <class Sndr, class Rcvr>
    using connect_result_t =
        decltype(connect(std::declval<Sndr>(), std::declval<Rcvr>()));
};
