#pragma once
#include <utility>

#include "../__inplace_stop_callback.hpp"
#include "../__inplace_stop_token.hpp"
#include "../__inplace_stop_source.hpp"

namespace mcs::execution::stoptoken
{
    template <__detail::invocable_destructible CallbackFn>
    template <class Initializer>
    inline inplace_stop_callback<CallbackFn>::inplace_stop_callback(
        inplace_stop_token st,
        Initializer
            &&init) noexcept(std::is_nothrow_constructible_v<CallbackFn, Initializer>)
        : callback_fn(std::forward<Initializer>(init)),
          stop_source(const_cast<inplace_stop_source *>(st.stop_source)) // NOLINT
    {
        // 1. Constraints: constructible_from<CallbackFn, Initializer> is satisfied.
        static_assert(std::constructible_from<CallbackFn, Initializer>);
        // 2. Effects: Initializes callback-fn with std::forward<Initializer>(init) and
        // executes a stoppable callback registration
        if (stop_source != nullptr &&
            ::mcs::execution::stoptoken::inplace_stop_source::stop_possible())
        {
            if (not stop_source->stop_requested())
            {
                stop_source->registration(this);
                registered = true;
            }
            else
                std::forward<CallbackFn>(callback_fn)(); // immediately evaluated
        }
    }

    template <__detail::invocable_destructible CallbackFn>
    inline inplace_stop_callback<CallbackFn>::~inplace_stop_callback()
    {
        // Note: it is shall have no effect if no registered
        if (registered)
        {
            // shall be removed from the associated stop state.
            stop_source->deregistration(this);
        }
        // The stoppable callback deregistration shall destroy callback_fn
    }

    template <__detail::invocable_destructible CallbackFn>
    auto inline inplace_stop_callback<CallbackFn>::invoke_callback() -> void
    {
        std::forward<CallbackFn>(callback_fn)();
    }

}; // namespace mcs::execution::stoptoken