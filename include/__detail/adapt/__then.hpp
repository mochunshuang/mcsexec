#pragma once

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../recv/__set_error.hpp"
#include "../recv/__set_stopped.hpp"
#include "../recv/__set_value.hpp"

#include "../cmplsigs/__eptr_completion_if.hpp"
#include "../diagnostics/__check.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        template <typename Completion>
        struct __then_t
        {
            // make_sender provides tag_of_t will-format
            template <snd::sender Sndr, movable_value Fun>
                requires(diagnostics::check_type<snd::__detail::basic_sender<
                             adapt::__then_t<Completion>, std::decay_t<Fun>,
                             std::decay_t<Sndr>>>)
            auto operator()(Sndr &&sndr, Fun &&f) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(dom,
                                             snd::make_sender(*this, std::forward<Fun>(f),
                                                              std::forward<Sndr>(sndr)));
            }

            template <movable_value Fun>
                requires(diagnostics::check_type<__then_t<Completion>, std::decay_t<Fun>>)
            auto operator()(Fun &&fun) const -> pipeable::sender_adaptor<__then_t, Fun>
            {
                return {*this, std::forward<Fun>(fun)};
            }
        };

        using then_t = __then_t<recv::set_value_t>;
        using upon_error_t = __then_t<recv::set_error_t>;
        using upon_stopped_t = __then_t<recv::set_stopped_t>;

        inline constexpr then_t then{};                 // NOLINT
        inline constexpr upon_error_t upon_error{};     // NOLINT
        inline constexpr upon_stopped_t upon_stopped{}; // NOLINT

    }; // namespace adapt

    template <typename Completion>
    struct snd::general::impls_for<adapt::__then_t<Completion>>
        : snd::__detail::default_impls
    {
        static constexpr auto complete = // NOLINT
            []<class Fn, class Tag, class... Args>(auto, Fn &fn, auto &rcvr, Tag,
                                                   Args &&...args) noexcept -> void {
            // Note: only handle same Completion or forward
            // Note: using the result value of f as then-cpo(sndr, f) value completion
            if constexpr (std::same_as<Tag, Completion>)
            {
                constexpr bool nothrow = // NOLINT
                    noexcept(std::invoke(std::move(fn), std::forward<Args>(args)...));

                if constexpr (nothrow)
                {
                    if constexpr (std::is_void_v<std::invoke_result_t<Fn, Args...>>)
                    {
                        std::invoke(std::move(fn), std::forward<Args>(args)...);
                        recv::set_value(std::move(rcvr));
                    }
                    else
                    {
                        recv::set_value(
                            std::move(rcvr),
                            std::invoke(std::move(fn), std::forward<Args>(args)...));
                    }
                }
                else
                {
                    try
                    {
                        if constexpr (std::is_void_v<std::invoke_result_t<Fn, Args...>>)
                        {
                            std::invoke(std::move(fn), std::forward<Args>(args)...);
                            recv::set_value(std::move(rcvr));
                        }
                        else
                        {
                            recv::set_value(
                                std::move(rcvr),
                                std::invoke(std::move(fn), std::forward<Args>(args)...));
                        }
                    }
                    catch (...)
                    {
                        recv::set_error(std::move(rcvr), std::current_exception());
                    }
                }
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    template <typename Completion, typename Fun, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::__then_t<Completion>, Fun, Sndr>, Env...>
    {
        static constexpr auto transform = // NOLINT
            []<class Tag, class... As>(Tag (*)(As...)) {
                if constexpr (std::is_same_v<Tag, Completion>)
                {
                    using T = decltype(std::declval<Fun>()(std::declval<As>()...));
                    constexpr bool nothrow = // NOLINT
                        noexcept(std::declval<Fun>()(std::declval<As>()...));
                    if constexpr (std::is_same_v<T, void>)
                        return cmplsigs::completion_signatures<set_value_t()>{} +
                               eptr_completion_if<nothrow>;
                    else
                        return cmplsigs::completion_signatures<set_value_t(T)>{} +
                               eptr_completion_if<nothrow>;
                }
                else
                    return cmplsigs::completion_signatures<Tag(As...)>{};
            };
        using type =
            decltype(snd::completion_signatures_of_t<Sndr, Env...>::transform_sigs(
                transform));
    };

    namespace diagnostics
    {
        template <typename Completion, class Fun> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<adapt::__then_t<Completion>, Fun> =
            []() consteval {
                if constexpr (std::is_same_v<Completion, set_error_t>)
                {
                    return check_set_error_arg<Fun>;
                }
                else if constexpr (std::is_same_v<Completion, set_stopped_t>)
                    return check_set_stoped_arg<Fun>;
                else
                    return true;
            }();

        template <typename Completion, class Sndr, class Fun,
                  class... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<adapt::__then_t<Completion>, Fun, Sndr>,
            Env...> = check_type_impl<adapt::__then_t<Completion>, Fun> && []() consteval {
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto fn = []<class... Ts>(Completion (*)(Ts...)) {
                if constexpr (!std::invocable<Fun, Ts...>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::__then_t<Completion>), WITH_SENDER(Sndr),
                        WITH_FUNCTION(Fun), WITH_ARGUMENTS(Ts...),
                        NOTE_INFO(
                            The_previous_completion_signature_does_not_match_the_current_function)>();
            };
            CS::check_sigs(overload_set{fn, [](auto) {
                                        }});
            return true;
        }();

    }; // namespace diagnostics

}; // namespace mcs::execution