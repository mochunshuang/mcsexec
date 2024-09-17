#pragma once

#include <exception>

#include "../../awaitables/__with_await_transform.hpp"

#include "../../__core_types.hpp"
#include "../../queries/__env_of_t.hpp"

namespace mcs::execution::conn::__detail
{

    template <class DS, class DR>
    struct connect_awaitable_promise
        : awaitables::with_await_transform<connect_awaitable_promise<DS, DR>>
    {
        struct operation_state_task // NOLINT
        {
            using operation_state_concept = operation_state_t;
            using promise_type = connect_awaitable_promise;

            explicit operation_state_task(std::coroutine_handle<> h) noexcept : coro(h) {}

            operation_state_task(operation_state_task &&o) noexcept
                : coro(std::exchange(o.coro, {}))
            {
            }

            ~operation_state_task()
            {
                if (coro)
                    coro.destroy();
            }

            void start() & noexcept
            {
                coro.resume();
            }

          private:
            std::coroutine_handle<> coro; // exposition only // NOLINT
        };

        connect_awaitable_promise(DS & /*unused*/, DR &rcvr) noexcept : rcvr(rcvr) {}

        std::suspend_always initial_suspend() noexcept // NOLINT
        {
            return {};
        }
        [[noreturn]] std::suspend_always final_suspend() noexcept // NOLINT
        {
            std::terminate();
        }
        [[noreturn]] void unhandled_exception() noexcept // NOLINT
        {
            std::terminate();
        }
        [[noreturn]] void return_void() noexcept // NOLINT
        {
            std::terminate();
        }

        std::coroutine_handle<> unhandled_stopped() noexcept // NOLINT
        {
            set_stopped(std::move(rcvr));
            return std::noop_coroutine();
        }

        operation_state_task get_return_object() noexcept // NOLINT
        {
            return operation_state_task{
                std::coroutine_handle<connect_awaitable_promise>::from_promise(*this)};
        }

        queries::env_of_t<DR> get_env() const noexcept // NOLINT
        {
            return queries::get_env(rcvr);
        }

      private:
        DR &rcvr; // exposition only // NOLINT
    };
}; // namespace mcs::execution::conn::__detail