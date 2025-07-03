#pragma once

#include "../__stoptoken/__inplace_stop_source.hpp"

namespace mcs::execution::__task
{
    template <typename>
    struct stop_source_of
    {
        using type = stoptoken::inplace_stop_source;
    };
    template <typename Context>
        requires requires { typename Context::stop_source_type; }
    struct stop_source_of<Context>
    {
        using type = typename Context::stop_source_type;
    };
    template <typename Context>
    using stop_source_of_t = typename stop_source_of<Context>::type;

}; // namespace mcs::execution::__task