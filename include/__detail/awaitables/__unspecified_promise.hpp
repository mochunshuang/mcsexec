#pragma once
#include <coroutine>

namespace mcs::execution::awaitables
{

    struct unspecified_promise
    {
        auto get_return_object() noexcept -> unspecified_promise;       // NOLINT
        auto initial_suspend() noexcept -> ::std::suspend_never;        // NOLINT
        auto final_suspend() noexcept -> ::std::suspend_never;          // NOLINT
        void unhandled_exception() noexcept;                            // NOLINT
        void return_void() noexcept;                                    // NOLINT
        auto unhandled_stopped() noexcept -> ::std::coroutine_handle<>; // NOLINT
    };
}; // namespace mcs::execution::awaitables