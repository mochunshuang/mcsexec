#pragma once

#include "./__is_awaitable.hpp"

namespace mcs::execution::awaitables
{
    template <class T, class Promise>
    concept has_as_awaitable = // exposition only
        requires(T &&t, Promise &p) {
            { std::forward<T>(t).as_awaitable(p) } -> is_awaitable<Promise &>;
        };
}; // namespace mcs::execution::awaitables
