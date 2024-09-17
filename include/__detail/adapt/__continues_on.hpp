#pragma once

#include "../sched/__scheduler.hpp"
#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__JOIN_ENV.hpp"
#include "../snd/general/__SCHED_ATTRS.hpp"
#include "../snd/general/__FWD_ENV.hpp"
#include "../snd/general/__get_domain_early.hpp"
#include <utility>

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
        struct continues_on_t
        {
            // make_sender provides tag_of_t will-format
            template <typename Sndr, typename Sch>
                requires(snd::sender<Sndr> && sched::scheduler<Sch>)
            auto operator()(Sndr &&sndr, Sch &&sch) const
            {
                return snd::transform_sender(snd::general::get_domain_early(sndr),
                                             snd::make_sender(this,
                                                              std::forward<Sch>(sch),
                                                              std::forward<Sndr>(sndr)));
            }
        };
        inline constexpr continues_on_t continues_on{}; // NOLINT

    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::continues_on_t> : snd::__detail::default_impls
    {
        static constexpr auto get_attrs = // NOLINT
            [](const auto &data, const auto &child) noexcept -> decltype(auto) {
            return snd::general::JOIN_ENV(snd::general::SCHED_ATTRS(data),
                                          snd::general::FWD_ENV(get_env(child)));
        };
    };

}; // namespace mcs::execution
