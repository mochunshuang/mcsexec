
#pragma once

#include <utility>

namespace mcs::execution::snd::__detail::mate_type
{
    template <class Sndr, std::size_t I = 0>
    using child_type =
        decltype(std::declval<Sndr>().template get<I + 2>()); // exposition only

}; // namespace mcs::execution::snd::__detail::mate_type
