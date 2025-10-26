#pragma once

#include <memory>

namespace mcs::execution::task_v2::__detail
{
    template <typename Environment>
    using get_allocator_type = typename decltype([] consteval {
        if constexpr (requires { typename Environment::allocator_type; })
        {
            static_assert(
                requires {
                    typename std::allocator_traits<typename Environment::allocator_type>::
                        template rebind_alloc<std::byte>;
                }, "allocator_type shall meet the Cpp17Allocator requirements");
            return std::type_identity<typename Environment::allocator_type>{};
        }
        else
            return std::type_identity<std::allocator<std::byte>>{};
    }())::type;

}; // namespace mcs::execution::task_v2::__detail