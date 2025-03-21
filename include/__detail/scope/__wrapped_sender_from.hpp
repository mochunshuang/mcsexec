#pragma once

#include "./__async_scope_token.hpp"

#include "../snd/__sender.hpp"

namespace mcs::execution::scope
{
    template <scope::async_scope_token Token, snd::sender Sender>
    using wrapped_sender_from = std::decay_t<decltype(std::declval<Token &>().wrap(
        std::declval<Sender>()))>; // exposition-only

}; // namespace mcs::execution::scope