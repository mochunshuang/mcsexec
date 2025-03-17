#pragma once

#include "../snd/__make_sender.hpp"

#include "../snd/general/__impls_for.hpp"

#include "../queries/__get_env.hpp"
#include <type_traits>
#include <utility>

#include "../cmplsigs/__eptr_completion_if.hpp"
#include "../diagnostics/__check.hpp"

namespace mcs::execution
{

    namespace factories
    {
        /**
         * read_env is a sender factory for a sender whose asynchronous operation
         * completes synchronously in its start operation with a value completion result
         * equal to a value read from the receiver’s associated environment.
         */
        struct read_env_t
        {
            template <typename Q>
                requires(diagnostics::check_type<snd::__detail::basic_sender<
                             factories::read_env_t, std::decay_t<Q>>>)
            snd::sender auto constexpr operator()(Q &&q) const noexcept
            {
                return snd::make_sender(*this, std::forward<Q>(q));
            }
        };
        inline constexpr read_env_t read_env{}; // NOLINT
    }; // namespace factories

    template <>
    struct snd::general::impls_for<factories::read_env_t> : snd::__detail::default_impls
    {
        static constexpr auto start = // NOLINT
            [](auto query, auto &rcvr) noexcept -> void {
            recv::set_value(std::move(rcvr), query(queries::get_env(rcvr)));
        };
    };

    template <typename Q, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<factories::read_env_t, Q>, Env>
    {
        using type = cmplsigs::completion_signatures<recv::set_value_t(
            decltype(Q()(std::declval<Env>())))>;
    };

    namespace diagnostics
    {
        template <class Q, class... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<factories::read_env_t, Q>,
            Env...> = []() consteval {
            if constexpr (sizeof...(Env) == 0)
                return true;
            else if constexpr (requires {
                                   { Q()(std::declval<Env>()...) } noexcept;
                               })
            {
                using T = decltype(Q()(std::declval<Env>()...));
                if constexpr (std::is_void_v<T>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(factories::read_env_t), WITH_ENV(Env...),
                        WITH_ARGUMENTS(Q),
                        NOTE_INFO(
                            the_query_funcation_of_the_env_need_nothrow_and_invokeable_with_the_given_query_type)>();
                else
                    return true;
            }
            else
                return false;
        }();
    }; // namespace diagnostics

}; // namespace mcs::execution
