
#pragma once
#include "../__completion_signatures.hpp"
#include <tuple>

namespace mcs::execution::cmplsigs::__detail
{
    template <typename Tag, typename Tuple>
    struct build_sig_from_args;

    template <typename Tag, typename... T>
    struct build_sig_from_args<Tag, std::tuple<T...>>
    {
        using type = Tag(T...);
    };
}; // namespace mcs::execution::cmplsigs::__detail