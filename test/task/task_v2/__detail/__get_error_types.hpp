#pragma once

#include "../../../test_base_head.hpp"

namespace mcs::execution::task_v2::__detail
{
    template <typename Environment>
    using get_error_types = typename decltype([] consteval {
        if constexpr (requires { typename Environment::error_types; })
            return std::type_identity<typename Environment::error_types>{};
        else
            return std::type_identity<
                cmplsigs::completion_signatures<recv::set_error_t(std::exception_ptr)>>{};
    }())::type;

}; // namespace mcs::execution::task_v2::__detail