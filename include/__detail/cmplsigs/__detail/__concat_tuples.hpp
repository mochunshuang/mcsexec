#pragma once

#include <tuple>

namespace mcs::execution::cmplsigs::__detail
{
    template <typename... Tuples>
    struct concat_tuples
    {
        using type = decltype(std::tuple_cat(Tuples{}...));
    };
}; // namespace mcs::execution::cmplsigs::__detail