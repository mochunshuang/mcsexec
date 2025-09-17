#pragma once

#include <type_traits>
#include <utility>

#include "./__let_value.hpp"

#include "../cmplsigs/__eptr_completion_if.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        // starts_on adapts an input sender into a sender that will start on an execution
        // agent belonging to a particular scheduler’s associated execution resource.
        // Note: execution::starts_on algorithm will ensure that the given sender will
        // Note: [ start in the specified context ], and doesn’t care where the
        // Note: completion-signal for that sender is sent.
        struct starts_on_t
        {
            template <sched::scheduler Sched, snd::sender Sndr>
            auto operator()(Sched &&sch, Sndr &&sndr) const noexcept
            {
                auto dom = snd::general::query_or_default(
                    queries::get_domain, std::as_const(sch), snd::default_domain());
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sched>(sch),
                                          std::forward<Sndr>(sndr)));
            };

            // for default_domain
            template <snd::sender_for<starts_on_t> Sndr, typename Env>
            auto transform_env(Sndr &&out_sndr, Env &&env) noexcept // NOLINT
            {
                auto &&[_, sch, __] = std::forward<Sndr>(out_sndr);
                return snd::general::JOIN_ENV(
                    snd::general::SCHED_ENV(sch),
                    snd::general::FWD_ENV(std::forward<Env>(env)));
            }

            // for connect
            template <snd::sender_for<starts_on_t> Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&out_sndr, const Env & /*env*/) noexcept
            {
                // Note: optimization for no copy. @see example/bulk.cpp test
                auto &&[_, sch, sndr] = std::forward<Sndr>(out_sndr);
                return adapt::let_value(
                    factories::schedule(std::forward_like<Sndr>(sch)),
                    [s = std::forward_like<Sndr>(sndr)]() mutable noexcept(
                        std::is_nothrow_move_constructible_v<decltype(sndr)>) {
                        return std::move(s);
                    });
                ;
            }
        };

        inline constexpr starts_on_t starts_on{}; // NOLINT

    }; // namespace adapt

    template <typename Sched, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::starts_on_t, Sched, Sndr>, Env...>
    {

        using type = decltype(snd::completion_signatures_of_t<Sndr, Env...>{} +
                              cmplsigs::eptr_completion_if<
                                  std::is_nothrow_move_constructible_v<Sndr>>);
    };

}; // namespace mcs::execution