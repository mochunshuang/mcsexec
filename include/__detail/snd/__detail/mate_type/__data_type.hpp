
#pragma once

#include <utility>

namespace mcs::execution::snd::__detail::mate_type
{
    template <class Sndr>
    using data_type = decltype(std::declval<Sndr>().template get<1>()); // exposition only

}; // namespace mcs::execution::snd::__detail::mate_type
