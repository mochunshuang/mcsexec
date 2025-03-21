#pragma once

#include <utility>

#include "../snd/__sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/__get_completion_signatures.hpp"

#include "../snd/__detail/__basic_sender.hpp"

#include "../cmplsigs/__completion_signatures_for.hpp"

#include "../diagnostics/__check_type.hpp"

namespace mcs::execution
{
    namespace factories
    {
        struct write_env_t
        {
            /**
             * write_env is sender adaptor that accepts a sender and a queryable object,
             * and that returns a sender that, when connected with a receiver rcvr,
             * connects the adapted sender with a receiver whose execution environment is
             * the result of joining the queryable object to the result of get_env(rcvr).
             */
            template <snd::sender Sndr, queryable Env>
                requires(
                    diagnostics::check_type<snd::__detail::basic_sender<
                        factories::write_env_t, std::decay_t<Env>, std::decay_t<Sndr>>>)
            constexpr auto operator()(Sndr &&sndr, Env &&env) const noexcept
            {
                return snd::make_sender(*this, std::forward<Env>(env),
                                        std::forward<Sndr>(sndr));
            };
        };
        inline constexpr write_env_t write_env{}; // NOLINT

        template <typename State, typename Env>
        struct write_env_env_t
        {
            template <typename Q>
            constexpr auto query(Q &&q) const noexcept
                requires(
                    requires { std::declval<State>().query(std::forward<Q>(q)); } ||
                    requires { std::declval<Env>().query(std::forward<Q>(q)); })
            {
                if constexpr (requires { state.query(std::forward<Q>(q)); })
                    return state.query(std::forward<Q>(q));
                else
                    return env.query(std::forward<Q>(q));
            }
            const State &state; // NOLINT
            Env env;            // NOLINT
        };
    }; // namespace factories

    template <>
    struct snd::general::impls_for<factories::write_env_t> : snd::__detail::default_impls
    {
        // NOLINTNEXTLINE
        static constexpr auto join_env(const auto &state, const auto &env) noexcept
        {
            return factories::write_env_env_t{state, env};
        }
        static constexpr auto get_env = // NOLINT
            [](auto, const auto &state, const auto &rcvr) noexcept {
                // NOTE: 现在CS都是编译期确定了。rcvr是运行期传来的，不会太慢的
                return join_env(state, queries::get_env(rcvr));
            };
    };

    template <typename NewEnv, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<factories::write_env_t, NewEnv, Sndr>, Env...>
    {
        // NOTE: get_completion_signatures_impl 优先匹配Env...
        // NOTE: 再次是 Env==1,但是 Env...不传也能够计算的，也算合格，因此Env可以可有可无
        using State = NewEnv;
        using type =
            decltype(snd::get_completion_signatures<
                     Sndr,
                     decltype(snd::general::impls_for<factories::write_env_t>::join_env(
                         std::declval<State>(), std::declval<Env>()))...>());
    };

    namespace diagnostics
    {
        template <class Sndr, class Data, class... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<factories::write_env_t, Data, Sndr>,
            Env...> = []() consteval {
            static_cast<void>(
                snd::get_completion_signatures<
                    Sndr,
                    decltype(snd::general::impls_for<factories::write_env_t>::join_env(
                        std::declval<Data>(), std::declval<Env>()))...>());
            return true;
        }();
    }; // namespace diagnostics
}; // namespace mcs::execution