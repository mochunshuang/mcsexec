#pragma once

#include "./__is_specialization.hpp"
#include <coroutine>

namespace mcs::execution::awaitables
{
    template <class T>
    concept await_suspend_result = // exposition only
        std::is_void_v<T> || std::is_same_v<T, bool> ||
        __detail::__is_specialization_v<T, std::coroutine_handle>;

}; // namespace mcs::execution::awaitables
