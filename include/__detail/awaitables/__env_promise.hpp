#pragma once

#include "./__with_await_transform.hpp"

namespace mcs::execution::awaitables
{
    // Specializations of env-promise are only used for the purpose of type
    // computation; its members need not be defined
    template <class Env>
    struct env_promise : with_await_transform<env_promise<Env>>
    {
        auto get_return_object() noexcept;                    // NOLINT
        auto initial_suspend() noexcept;                      // NOLINT
        auto final_suspend() noexcept;                        // NOLINT
        void unhandled_exception() noexcept;                  // NOLINT
        void return_void() noexcept;                          // NOLINT
        std::coroutine_handle<> unhandled_stopped() noexcept; // NOLINT
        const Env &get_env() const noexcept;                  // NOLINT
    };
}; // namespace mcs::execution::awaitables