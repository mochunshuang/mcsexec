#pragma once
#include <utility>
#include "./__is_awaitable.hpp"
#include "./__sender_awaitable.hpp"
#include "./__awaitable_sender.hpp"
#include "./__has_as_awaitable.hpp"

namespace mcs::execution::awaitables
{

    struct as_awaitable_t
    {
        struct unspecified_class;

        template <typename Expr, typename Promise>
        auto operator()(Expr &&expr, Promise &p) const
        {
            // 1. expr.as_awaitable(p) if that expression is well-formed.
            if constexpr (has_as_awaitable<Expr, Promise>)
            {
                return ::std::forward<Expr>(expr).as_awaitable(p);
            }
            // 2. (void(p), expr) if is-awaitable<Expr, U> is true, where U is an
            // unspecified class type that is not Promise and that lacks a member
            // named await_transform.
            else if constexpr (awaitables::is_awaitable<Expr, unspecified_class>)
            {
                // Preconditions: is-awaitable<Expr, Promise> is true and the expression
                // co_await expr in a coroutine with promise type U is
                // expression-equivalent to the same expression in a coroutine with
                // promise type Promise.
                // Note:要求 await_suspend(std::coroutine_handle<Promise>) 和
                // await_suspend(std::coroutine_handle<unspecified_class>) 的行为是一样的
                // Note: 对await_suspend实现是软约束的，编译期无法约束
                // Note: 类型std::coroutine_handle<>可以接收任意 std::coroutine_handle<T>
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