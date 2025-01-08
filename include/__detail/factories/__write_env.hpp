#pragma once

#include <utility>

#include "../snd/general/__JOIN_ENV.hpp"

#include "../snd/__sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/__get_completion_signatures.hpp"

#include "../snd/__detail/__basic_sender.hpp"

#include "../cmplsigs/__completion_signatures_for.hpp"

namespace mcs::execution
{
    namespace factories
    {
        struct write_env_t
        {
            // TODO(mcs): 或许要更改
            template <snd::sender Sndr, queryable Env>
            constexpr auto operator()(Sndr &&sndr, Env &&env)
            {
                return snd::make_sender(*this, std::forward<Env>(env),
                                        std::forward<Sndr>(sndr));
            };
        };
        inline constexpr write_env_t write_env{}; // NOLINT

        template <typename State, typename Rcvr>
        struct write_env_env_t
        {
            template <typename Q>
            constexpr auto query(Q &&q) const noexcept
            {
                if constexpr (requires { state.query(std::forward<Q>(q)); })
                    return state.query(std::forward<Q>(q));
                else if constexpr (requires {
                                       queries::get_env(rcvr).query(std::forward<Q>(q));
                                   })
                    return queries::get_env(rcvr).query(std::forward<Q>(q));
                else
                    return empty_env{};
            }
            const State &state; // NOLINT
            const Rcvr &rcvr;   // NOLINT
        };
    }; // namespace factories

    template <>
    struct snd::general::impls_for<factories::write_env_t> : snd::__detail::default_impls
    {
        static constexpr auto get_env = // NOLINT
            [](auto, const auto &state, const auto &rcvr) noexcept {
                // return snd::general::JOIN_ENV(state, queries::get_env(rcvr));
                return factories::write_env_env_t{state, rcvr};
            };
    };

    template <typename NewEnv, typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<factories::write_env_t, NewEnv, Sndr>, Env>
    {
        using type = decltype(snd::get_completion_signatures(
            std::declval<Sndr>(),
            factories::write_env_env_t(std::declval<NewEnv>(), std::declval<Env>())));
    };

}; // namespace mcs::execution