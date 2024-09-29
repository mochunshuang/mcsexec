#pragma once

#include <type_traits>
#include <utility>

#include "./__let_value.hpp"

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
            auto operator()(Sched &&sch, Sndr &&sndr) const
            {
                auto dom = snd::general::query_or_default(
                    queries::get_domain, std::as_const(sch), snd::default_domain());
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sched>(sch),
                                          std::forward<Sndr>(sndr)));
            };

            // for default_domain
            template <snd::sender OutSndr, typename Env>
                requires(snd::sender_for<OutSndr, starts_on_t>)
            auto transform_env(OutSndr &&out_sndr, Env &&env) noexcept // NOLINT
            {
                auto &&[_, sch, __] = out_sndr;
                return snd::general::JOIN_ENV(
                    snd::general::SCHED_ENV(sch),
                    snd::general::FWD_ENV(std::forward<Env>(env)));
            }

            // for connect
            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&out_sndr, const Env & /*env*/) noexcept
                requires(snd::sender_for<decltype((out_sndr)), starts_on_t>)
            {
                // Note: optimization for no copy
                using OutSndr = decltype(out_sndr);
                auto &&[_, sch, sndr] = out_sndr;
                return adapt::let_value(
                    factories::schedule(sch),
                    [sndr = std::forward_like<OutSndr>(sndr)]() mutable noexcept(
                        std::is_nothrow_move_constructible_v<decltype(sndr)>) {
                        return std::move(sndr);
                    });
                ;
            }
        };

        inline constexpr starts_on_t starts_on{}; // NOLINT

    }; // namespace adapt

    template <typename Sched, typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::starts_on_t, Sched, Sndr>, Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

}; // namespace mcs::execution