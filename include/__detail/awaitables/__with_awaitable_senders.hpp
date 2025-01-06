#pragma once

#include <exception>

#include "./__as_awaitable.hpp"

namespace mcs::execution::awaitables
{
    /**
     * with_awaitable_senders, when used as the base class of a coroutine promise type,
     * makes senders awaitable in that coroutine type.
     * In addition, it provides a default implementation of unhandled_stopped() such that
     * if a sender completes by calling set_stopped, it is treated as if an uncatchable
     * "stopped" exception were thrown from the await-expression.
     * Note: The coroutine is never resumed, and the unhandled_stopped of the coroutine
     * Note: caller’s promise type is called.
     */
    template <class Promise>
    struct with_awaitable_senders
    {
        template <class OtherPromise>
            requires(not std::same_as<OtherPromise, void>)
        void set_continuation(std::coroutine_handle<OtherPromise> h) noexcept // NOLINT
        {
            continuation_ = h;
            if constexpr (requires(OtherPromise &other) { other.unhandled_stopped(); })
            {
                stopped_handler = [](void *p) noexcept -> std::coroutine_handle<> {
                    return std::coroutine_handle<OtherPromise>::from_address(p)
                        .promise()
                        .unhandled_stopped();
                };
            }
            else
            {
                stopped_handler = &default_unhandled_stopped;
            }
        }

        [[nodiscard]] std::coroutine_handle<> continuation() const noexcept
        {
            return continuation_;
        }

        std::coroutine_handle<> unhandled_stopped() noexcept // NOLINT
        {
            return stopped_handler(continuation_.address());
        }

        template <class Value>
        auto await_transform(Value &&value) // NOLINT
        {
            return awaitables::as_awaitable(std::forward<Value>(value),
                                            static_cast<Promise &>(*this));
        }

      private:
        // exposition only // NOLINTNEXTLINE
        [[noreturn]] static std::coroutine_handle<> default_unhandled_stopped(
            void * /*unused*/) noexcept
        {
            std::terminate();
        }
        std::coroutine_handle<> continuation_{}; // exposition only // NOLINT
        // exposition only // NOLINTNEXTLINE
        std::coroutine_handle<> (*stopped_handler)(void *) noexcept =
            &default_unhandled_stopped;
    };
}; // namespace mcs::execution::awaitables