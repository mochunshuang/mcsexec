#pragma once

#include <type_traits>

namespace mcs::execution::awaitables
{
    struct unit
    {
    }; // exposition only

    template <class value_type>
    using result_type = std::conditional_t<std::is_void_v<value_type>, unit, value_type>;

}; // namespace mcs::execution::awaitables