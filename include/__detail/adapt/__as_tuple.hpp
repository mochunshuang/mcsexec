#pragma once

#include "../__core_types.hpp"
namespace mcs::execution::adapt::__detail
{

    template <typename _Sig>
    struct as_tuple;
    template <typename Tag, typename... Args>
    struct as_tuple<Tag(Args...)>
    {
        using type = ::mcs::execution::decayed_tuple<Tag, Args...>;
    };

}; // namespace mcs::execution::adapt::__detail