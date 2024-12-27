#pragma once

namespace mcs::execution::stoptoken
{
    template <typename Token, typename CallbackFn>
    using stop_callback_of_t = typename Token::template callback_type<CallbackFn>;

}; // namespace mcs::execution::stoptoken