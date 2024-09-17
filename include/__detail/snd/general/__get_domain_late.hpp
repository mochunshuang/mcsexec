#pragma once

#include "./__query_or_default.hpp"
#include "./__completion_domain.hpp"

#include "../__sender_for.hpp"
#include "../__default_domain.hpp"

#include "../../queries/__get_env.hpp"
#include "../../queries/__get_domain.hpp"
#include "../../queries/__get_scheduler.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        struct continues_on_t;
    };
    using adapt::continues_on_t;

    namespace snd::general
    {
        ////////////////////////////////////////
        // get_domain_late
        template <class Sndr, class Env>
        constexpr auto get_domain_late(const Sndr &sndr, const Env &env) noexcept
        {
            using queries::get_domain;
            using queries::get_env;
            using queries::get_scheduler;
            if constexpr (snd::sender_for<Sndr, continues_on_t>)
            {
                return sndr.apply([]<typename Sch>(auto &, Sch &sch,
                                                   auto &...) -> decltype(auto) {
                    return query_or_default(queries::get_domain, sch, default_domain());
                });
            }
            else if constexpr (requires {
                                   { get_domain(get_env(sndr)) } -> not_void;
                               })
            {
                return get_domain(get_env(sndr));
            }
            else if constexpr (requires {
                                   { completion_domain<void>(sndr) } -> not_void;
                               })
            {
                return completion_domain<void>(sndr);
            }
            else if constexpr (requires {
                                   { get_domain(env) } -> not_void;
                               })
            {
                return get_domain(env);
            }
            else if constexpr (requires {
                                   { get_domain(get_scheduler(env)) } -> not_void;
                               })
            {
                return get_domain(get_scheduler(env));
            }
            else
            {
                return default_domain();
            }
        }
    }; // namespace snd::general

}; // namespace mcs::execution
