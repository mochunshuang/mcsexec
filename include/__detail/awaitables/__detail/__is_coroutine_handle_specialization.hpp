#pragma once

#include <coroutine>

namespace mcs::execution::awaitables::__detail
{

    template <typename>
    struct is_coroutine_handle_specialization : ::std::false_type
    {
    };
    template <typename T>
    struct is_coroutine_handle_specialization<::std::coroutine_handle<T>>
        : ::std::true_type
    {
    };

}; // namespace mcs::execution::awaitables::__detail