#pragma once

namespace mcs::execution::stoptoken
{
    template <class T, class CallbackFn>
    using stop_callback_for_t = T::template callback_type<CallbackFn>;

    // 33.3.3, class stop_token
    class stop_token; // NOLINT

    // 33.3.4, class stop_source
    class stop_source; // NOLINT

    // no-shared-stop-state indicator
    struct nostopstate_t;

    // 33.3.5, class template stop_callback
    template <class CallbackFn>
    class stop_callback; // NOLINT

    // [stoptoken.never], class never_stop_token
    class never_stop_token;

    // [stoptoken.inplace], class inplace_stop_token
    class inplace_stop_token;

    // [stopsource.inplace], class inplace_stop_source
    class inplace_stop_source;

    // [stopcallback.inplace], class template inplace_stop_callback
    template <class CallbackFn>
    class inplace_stop_callback;

}; // namespace mcs::execution::stoptoken