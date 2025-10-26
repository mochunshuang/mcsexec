#pragma once

#include "../../test_base_head.hpp"

namespace mcs::execution
{
    namespace task_v2
    {
        // affine_on adapts a sender into one that completes on the specified scheduler.
        struct affine_on_t
        {
            template <snd::sender Sndr, sched::scheduler Sched>
            auto operator()(Sndr &&sndr, Sched &&sched) const noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sched>(sched),
                                          std::forward<Sndr>(sndr)));
            }
            template <sched::scheduler Sch>
            auto operator()(Sch &&sch) const noexcept
                -> pipeable::sender_adaptor<affine_on_t, Sch>
            {
                return {*this, std::forward<Sch>(sch)};
            }
        };

        inline constexpr affine_on_t affine_on; // NOLINT
    }; // namespace task_v2

    template <>
    struct snd::general::impls_for<task_v2::affine_on_t> : snd::__detail::default_impls
    {
        // used by get_env()
        static constexpr auto get_attrs = // NOLINT
            [](const auto &data, const auto &child) noexcept -> decltype(auto) {
            return snd::general::JOIN_ENV(snd::general::SCHED_ATTRS(data),
                                          snd::general::FWD_ENV(queries::get_env(child)));
            // return JOIN-ENV(SCHED-ATTRS(data), FWD-ENV(get_env(child)));
        };
    };

    template <typename Sched, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<task_v2::affine_on_t, Sched, Sndr>, Env...>
    {
        using type = typename cmplsigs::completion_signatures_for_impl<
            snd::__detail::basic_sender<adapt::schedule_from_t, Sched, Sndr>,
            Env...>::type;
    };
}; // namespace mcs::execution