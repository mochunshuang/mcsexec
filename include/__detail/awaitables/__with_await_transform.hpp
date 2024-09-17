#pragma once

#include "./__is_awaitable.hpp"
#include <utility>
namespace mcs::execution::awaitables
{
    template <class T, class Promise>
    concept has_as_awaitable = // exposition only
        requires(T &&t, Promise &p) {
            { std::forward<T>(t).as_awaitable(p) } -> is_awaitable<Promise &>;
        };

    template <class Derived>
    struct with_await_transform
    {
        template <class T>
        T &&await_transform(T &&value) noexcept // NOLINT
        {
            return std::forward<T>(value);
        }

        template <has_as_awaitable<Derived> T>
        decltype(auto) await_transform(T &&value) noexcept( // NOLINT
            noexcept(std::forward<T>(value).as_awaitable(std::declval<Derived &>())))
        {
            return std::forward<T>(value).as_awaitable(static_cast<Derived &>(*this));
        }
    };
}; // namespace mcs::execution::awaitables