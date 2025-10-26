#pragma once

#include "../__task_scheduler.hpp"

namespace mcs::execution::task_v2::__detail
{
    template <typename Environment>
    using get_scheduler_type = typename decltype([] consteval {
        if constexpr (requires { typename Environment::scheduler_type; })
            return std::type_identity<typename Environment::scheduler_type>{};
        else
            return std::type_identity<task_scheduler>{}; // task_scheduler 应该是不对的
    }())::type;

}; // namespace mcs::execution::task_v2::__detail
