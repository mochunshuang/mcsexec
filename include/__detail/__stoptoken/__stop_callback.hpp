#pragma once

#include "./__stop_token.hpp"
#include "./__invocable_destructible.hpp"

namespace mcs::execution::stoptoken
{

    // [stopcallback.general]
    template <__detail::invocable_destructible CallbackFn>
    class stop_callback // NOLINT
    {
      public:
        using callback_type = CallbackFn;

        // 33.3.5.2, constructors and destructor
        // stop_callback is instantiated with an argument for the template parameter
        // CallbackFn that satisfies both invocable and destructible.
        // Remarks:
        // For a type Initializer,
        //  if stoppable-callback-for<CallbackFn,stop_token,Initializer> is satisfied,
        //     then stoppable-callback-for<CallbackFn,stop_token, Initializer> is modeled.
        //  The exposition-only callback-fn member is the associated callback function
        //  ([stoptoken.concepts]) of stop_callback<CallbackFn> objects.
        //
        // Constraints:
        // CallbackFn and CInitializer satisfy constructible_from<CallbackFn,Initializer>.
        // Effects:
        //  Initializes callback-fn with std::forward<CInitializer>(init)
        //  and executes a stoppable callback registration ([stoptoken.concepts]) .
        //      If a callback is registered with st's shared stop state, then *this
        //      acquires shared ownership of that stop state.
        template <class Initializer>
        explicit stop_callback(const stop_token &st, Initializer &&init) noexcept(
            std::is_nothrow_constructible_v<CallbackFn, Initializer>);

        template <class Initializer>
        explicit stop_callback(stop_token &&st, Initializer &&init) noexcept(
            std::is_nothrow_constructible_v<CallbackFn, Initializer>);

        // Effects:
        // Executes a stoppable callback deregistration ([stoptoken.concepts])
        // and releases ownership of the stop state, if any.
        ~stop_callback();

        stop_callback(const stop_callback &) = delete;
        stop_callback(stop_callback &&) = delete;
        stop_callback &operator=(const stop_callback &) = delete;
        stop_callback &operator=(stop_callback &&) = delete;

      private:
        CallbackFn callback_fn; // NOLINT // exposition only
    };

    template <class CallbackFn>
    stop_callback(stop_token, CallbackFn) -> stop_callback<CallbackFn>;

}; // namespace mcs::execution::stoptoken