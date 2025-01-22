#pragma once

#include <utility>

namespace mcs::execution::awaitables::__detail
{

    template <typename Expr, typename Promise>
    constexpr decltype(auto) GET_AWAITER(Expr &&expr, Promise &promise)
    {
        // 0. promise can await_transform do await_transform
        auto transform{[&]() -> decltype(auto) {
            if constexpr (requires {
                              promise.await_transform(::std::forward<Expr>(expr));
                          })
                return promise.await_transform(::std::forward<Expr>(expr));
            else
                return ::std::forward<Expr>(expr);
        }};
        // 1. first global co_await
        if constexpr (requires { operator co_await(transform()); })
        {

            static_assert(
                not requires { transform().operator co_await(); },
                "only one operator co_await is allowed");
            return operator co_await(transform());
        }
        // 2. has member co_await or transform() itself
        else if constexpr (requires { transform().operator co_await(); })
            return transform().operator co_await();
        else
            return transform();
    }
}; // namespace mcs::execution::awaitables::__detail