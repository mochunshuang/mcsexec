#pragma once

#include "./__stop_callback_for_t.hpp"
#include <concepts>

namespace mcs::execution::stoptoken
{
    template <class CallbackFn, class Token, class Initializer = CallbackFn>
    concept stoppable_callback_for =
        std::invocable<CallbackFn> && std::constructible_from<CallbackFn, Initializer> &&
        requires { typename stop_callback_for_t<Token, CallbackFn>; } &&
        std::constructible_from<stop_callback_for_t<Token, CallbackFn>, const Token &,
                                Initializer>;

}; // namespace mcs::execution::stoptoken
