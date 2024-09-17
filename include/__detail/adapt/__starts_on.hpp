#pragma once

#include <utility>

#include "../sched/__scheduler.hpp"

#include "../snd/__sender.hpp"
#include "../snd/__sender_for.hpp"
#include "../snd/__default_domain.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/general/__query_or_default.hpp"
#include "../snd/general/__JOIN_ENV.hpp"
#include "../snd/general/__SCHED_ENV.hpp"
#include "../snd/general/__FWD_ENV.hpp"

#include "../queries/__get_domain.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        // starts_on adapts an input sender into a sender that will start on an execution
        // agent belonging to a particular scheduler’s associated execution resource.
        struct starts_on_t
        {
            template <sched::scheduler Sched, snd::sender Sndr>
            auto operator()(Sched &sch, Sndr &sndr) const
            {
                return transform_sender(
                    snd::general::query_or_default(queries::get_domain, sch,
                                                   snd::default_domain()),
                    snd::make_sender(*this, sch, sndr));
            };

            template <snd::sender Sndr, typename Env>
                requires(snd::sender_for<Sndr, Env>)
            auto transform_env(Sndr &&sndr, Env &&env) noexcept // NOLINT
            {
                return std::forward<Sndr>(sndr).apply(
                    [&]<typename Sched>(auto &&, Sched &&sch, auto &&) {
                        return snd::general::JOIN_ENV(
                            snd::general::SCHED_ENV(sch),
                            snd::general::FWD_ENV(std::forward<Env>(env)));
                    });
            }
            template <snd::sender Sndr, typename Env>
                requires(snd::sender_for<Sndr, Env>)
            auto transform_sender(Sndr &&sndr, const Env &env) noexcept // NOLINT
            {
                // TODO(mcs): need let_value
                // return let_value();
            }
        };
        inline constexpr starts_on_t starts_on{}; // NOLINT
    }; // namespace adapt

}; // namespace mcs::execution