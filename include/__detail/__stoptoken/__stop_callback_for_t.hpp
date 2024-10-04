#pragma once

namespace mcs::execution::stoptoken
{
    template <class T, class CallbackFn>
    using stop_callback_for_t = T::template callback_type<CallbackFn>;
};