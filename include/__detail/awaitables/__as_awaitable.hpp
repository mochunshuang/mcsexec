#pragma once
#include <utility>
#include "./__is_awaitable.hpp"
#include "./__unspecified_promise.hpp"
#include "./__sender_awaitable.hpp"
#include "./__awaitable_sender.hpp"

namespace mcs::execution::awaitables
{

    struct as_awaitable_t
    {
        template <typename Expr, typename Promise>
        auto operator()(Expr &&expr, Promise &p) const
        {
            // 1. expr.as_awaitable(p) if that expression is well-formed.
            if constexpr (requires { ::std::forward<Expr>(expr).as_awaitable(p); })
            {
                static_assert(
                    awaitables::is_awaitable<
                        decltype(::std::forward<Expr>(expr).as_awaitable(p)), Promise>,
                    "as_awaitable must return an awaitable");
                return ::std::forward<Expr>(expr).as_awaitable(p);
            }
            // 2. (void(p), expr) if is-awaitable<Expr, U> is true, where U is an
            // unspecified class type that is not Promise and that lacks a member
            // named await_transform.
            else if constexpr (awaitables::is_awaitable<Expr, unspecified_promise>)
            {
                // Preconditions: is-awaitable<Expr, Promise> is true and the expression
                // co_await expr in a coroutine with promise type U is
                // expression-equivalent to the same expression in a coroutine with
                // promise type Promise.
                // TODO(mcs): 待完善
                static_assert(awaitables::is_awaitable<Expr, Promise>);
                return (void(p), std::forward<Expr>(expr));
            }
            // 3. Otherwise, sender-awaitable{expr, p} if awaitable-sender<Expr,Promise>
            // is true
            else if constexpr (awaitables::awaitable_sender<Expr, Promise>)
            {
                return sender_awaitable{std::forward<Expr>(expr), p};
            }
            // 4. Otherwise, (void(p), expr).
            else
            {
                return (void(p), expr);
            }
        }

    }; // namespace awaitables

    constexpr inline as_awaitable_t as_awaitable{}; // NOLINT
}; // namespace mcs::execution::awaitables