#pragma once

#include <coroutine>
#include <utility>

namespace mcs::execution::conn::__detail
{
    template <class Fun, class... Ts>
    constexpr auto suspend_complete(Fun fun, Ts &&...as) noexcept // exposition only
    {
        auto fn = [&, fun]() noexcept {
            fun(std::forward<Ts>(as)...);
        };

        struct awaiter
        {
            decltype(fn) fn; // NOLINT

            static constexpr bool await_ready() noexcept // NOLINT
            {
                return false;
            }
            void await_suspend(std::coroutine_handle<>) noexcept // NOLINT
            {
                fn();
            }
            [[noreturn]] void await_resume() noexcept // NOLINT
            {
                std::unreachable();
            }
        };
        return awaiter{fn};
    }
}; // namespace mcs::execution::conn::__detail