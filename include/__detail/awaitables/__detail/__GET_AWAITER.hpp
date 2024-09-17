#pragma once

#include <utility>

namespace mcs::execution::awaitables::__detail
{
    // This functions returns the value of applying co_await (awaitable), when called
    // in a coroutine a specified promise type. C++ first tries to call a member
    // operator co_await, then a global operator co_await, and finally returns the
    // awaitable unmodified.
    template <typename Awaitable>
    constexpr decltype(auto) GET_AWAITER(Awaitable &&awaitable, void * /*unused*/)
    {
        if constexpr (requires {
                          std::forward<Awaitable>(awaitable).operator co_await();
                      })
        {
            return std::forward<Awaitable>(awaitable).operator co_await();
        }
        else if constexpr (requires {
                               operator co_await(std::forward<Awaitable>(awaitable));
                           })
        {
            return operator co_await(std::forward<Awaitable>(awaitable));
        }
        else
        {
            return std::forward<Awaitable>(awaitable);
        }
    }

    // When a concrete promise type is known, and it has a member await_transform(),
    // C++ calls that function before applying the above rules.
    template <typename Awaitable, typename Promise>
    constexpr decltype(auto) GET_AWAITER(Awaitable &&awaitable, Promise &promise)
        requires(requires {
            promise->await_transform(std::forward<Awaitable>(awaitable));
        })
    {
        return GET_AWAITER(promise->await_transform(std::forward<Awaitable>(awaitable)),
                           static_cast<void *>(&promise));
    }
}; // namespace mcs::execution::awaitables::__detail