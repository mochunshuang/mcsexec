
#pragma once

#include <type_traits>

namespace mcs::execution::snd::__detail::mate_type
{
    template <class Sndr>
    using indices_for = std::remove_reference_t<Sndr>::indices_for; // exposition only

}; // namespace mcs::execution::snd::__detail::mate_type
