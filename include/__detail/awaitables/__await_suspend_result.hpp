#pragma once

#include "./__detail/__is_coroutine_handle_specialization.hpp"

namespace mcs::execution::awaitables
{
    template <class T>
    concept await_suspend_result = // exposition only
        std::is_void_v<T> || std::is_same_v<T, bool> ||
        __detail::is_coroutine_handle_specialization<T>::value;

}; // namespace mcs::execution::awaitables
