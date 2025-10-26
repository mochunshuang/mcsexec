#pragma once

#include "../../../test_base_head.hpp"

namespace mcs::execution::task_v2::__detail
{
    template <typename Environment>
    using get_stop_source_type = typename decltype([] consteval {
        if constexpr (requires { typename Environment::stop_source_type; })
            return std::type_identity<typename Environment::stop_source_type>{};
        else
            return std::type_identity<stoptoken::inplace_stop_source>{};
    }())::type;

}; // namespace mcs::execution::task_v2::__detail