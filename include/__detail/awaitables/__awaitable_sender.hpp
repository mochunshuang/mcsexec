#pragma once

#include <concepts>
#include <coroutine>
#include "../queries/__env_of_t.hpp"
#include "../snd/__single_sender.hpp"
#include "../snd/__sender_to.hpp"
#include "./__awaitable_receiver.hpp"

namespace mcs::execution::awaitables
{
    template <class Sndr, class Promise>
    concept awaitable_sender =
        snd::single_sender<Sndr, queries::env_of_t<Promise>> &&
        snd::sender_to<Sndr,
                       awaitables::awaitable_receiver<Sndr, Promise>> && // see below
        requires(Promise &p) {
            { p.unhandled_stopped() } -> std::convertible_to<std::coroutine_handle<>>;
        };
}; // namespace mcs::execution::awaitables