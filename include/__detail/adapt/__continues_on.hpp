#pragma once

#include <utility>
#include "./__schedule_from.hpp"

#include "../snd/general/__get_domain_early.hpp"

#include "../pipeable/__sender_adaptor.hpp"

namespace mcs::execution
{

    namespace adapt
    {
        /////////////////////////////////////////////
        // [exec.continues.on]
        // continues_on是 使发送方适应在指定调度程序上完成 的发送方
        // The name continues_on denotes a pipeable sender adaptor object. For
        // subexpressions sch and sndr, if decltype((sch)) does not satisfy scheduler, or
        // decltype((sndr)) does not satisfy sender, continues_on(sndr, sch) is
        // ill-formed.
        // Note: The execution::continues_on algorithm will not care where the given
        // Note: sender is going to be [ started ], but will ensure that the
        // Note: completion-signal Note: of will be transferred to the given context
        struct continues_on_t
        {
            // make_sender provides tag_of_t will-format
            template <snd::sender Sndr, sched::scheduler Sch>
            auto operator()(Sndr &&sndr, Sch &&sch) const
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sch>(sch),
                                          std::forward<Sndr>(sndr)));
            }

            template <sched::scheduler Sch>
            auto operator()(Sch &&sch) const
                -> pipeable::sender_adaptor<continues_on_t, Sch>
            {
                return {*this, std::forward<Sch>(sch)};
            }

            // Note: The default implementation of continues_on(snd, sched) is
            // Note: schedule_from(sched, snd).
            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&sndr, const Env & /*env*/) noexcept
                requires(snd::sender_for<decltype((sndr)), continues_on_t>)
            {
                // Note: optimization for no copy
                using OutSndr = decltype(sndr);
                auto &&[_, data, child] = sndr;
                return schedule_from(std::forward_like<OutSndr>(data),
                                     std::forward_like<OutSndr>(child));
            }
        };

        inline constexpr continues_on_t continues_on{}; // NOLINT

    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::continues_on_t> : snd::__detail::default_impls
    {
        // used by get_env()
        static constexpr auto get_attrs = // NOLINT
            [](const auto &data, const auto &child) noexcept -> decltype(auto) {
            return snd::general::JOIN_ENV(snd::general::SCHED_ATTRS(data),
                                          snd::general::FWD_ENV(queries::get_env(child)));
        };
    };

    template <typename Sched, typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::continues_on_t, Sched, Sndr>, Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

}; // namespace mcs::execution
