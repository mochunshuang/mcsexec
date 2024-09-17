#pragma once

#include "../__core_types.hpp"
#include "../__core_concepts.hpp"
#include "../awaitables/__env_promise.hpp"
#include "../awaitables/__is_awaitable.hpp"
#include "../queries/__get_env.hpp"

namespace mcs::execution::snd
{

    //////////////////////////////////////////////////////////////////////////////
    // [exec.sender.concepts]
    template <class Sndr>
    concept is_sender = // exposition only
        std::derived_from<typename Sndr::sender_concept, sender_t>;

    template <class Sndr>
    concept enable_sender = // exposition only
        is_sender<Sndr> ||
        awaitables::is_awaitable<
            Sndr,
            awaitables::env_promise<::mcs::execution::empty_env>>; // [exec.awaitables]

    template <class Sndr>
    concept sender =                                      // NOLINTNEXTLINE
        bool(enable_sender<std::remove_cvref_t<Sndr>>) && // atomic constraint
                                                          // ([temp.constr.atomic])
        requires(const std::remove_cvref_t<Sndr> &sndr) {
            { queries::get_env(sndr) } -> queryable;
        } && std::move_constructible<std::remove_cvref_t<Sndr>> && // senders are
                                                                   // movable and
        std::constructible_from<std::remove_cvref_t<Sndr>,
                                Sndr>; // decay copyable

}; // namespace mcs::execution::snd