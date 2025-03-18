#pragma once

#include <execution>
#include <utility>

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../cmplsigs/__eptr_completion_if.hpp"
#include "../diagnostics/__check.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        template <typename T>
        concept shape = std::integral<std::remove_cvref_t<T>>;

        template <typename Policy>
        concept policy = std::is_execution_policy_v<std::remove_cvref_t<Policy>>;

        namespace __detail
        {
            // bulk runs a task repeatedly for every index in an index space.
            template <typename Algo>
            struct bulk_algo
            {
              private:
                bulk_algo() = default; // Note: for CRTP

              public:
                template <snd::sender Sndr, policy Policy, shape Shape, movable_value Fun>
                    requires(diagnostics::check_type<snd::__detail::basic_sender<
                                 Algo,
                                 snd::__detail::product_type<std::decay_t<Policy>,
                                                             std::decay_t<Shape>,
                                                             std::decay_t<Fun>>,
                                 std::decay_t<Sndr>>>)
                auto operator()(Sndr &&sndr, Policy &&policy, Shape &&shape,
                                Fun &&f) const // noexcept
                {
                    auto dom = snd::general::get_domain_early(std::as_const(sndr));
                    return snd::transform_sender(
                        dom, snd::make_sender(
                                 Algo(),
                                 snd::__detail::product_type{std::forward<Policy>(policy),
                                                             std::forward<Shape>(shape),
                                                             std::forward<Fun>(f)},
                                 std::forward<Sndr>(sndr)));
                }

                template <policy Policy, shape Shape, movable_value Fun>
                auto operator()(Policy &&policy, Shape &&shape, Fun &&fun) const
                    -> pipeable::sender_adaptor<bulk_algo, Policy, Shape, Fun>
                {
                    return {*this, std::forward<Policy>(policy),
                            std::forward<Shape>(shape), std::forward<Fun>(fun)};
                }
                friend Algo; // Note: for CRTP
            };

        }; // namespace __detail

        struct bulk_t : public __detail::bulk_algo<bulk_t>
        {
            using __detail::bulk_algo<bulk_t>::bulk_algo;
        };

        struct bulk_chunked_t : public __detail::bulk_algo<bulk_chunked_t>
        {
            using __detail::bulk_algo<bulk_chunked_t>::bulk_algo;
        };
        struct bulk_unchunked_t : public __detail::bulk_algo<bulk_unchunked_t>
        {
            using __detail::bulk_algo<bulk_unchunked_t>::bulk_algo;
        };

        inline constexpr bulk_t bulk{};                     // NOLINT
        inline constexpr bulk_chunked_t bulk_chunked{};     // NOLINT
        inline constexpr bulk_unchunked_t bulk_unchunked{}; // NOLINT
    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::bulk_unchunked_t> : snd::__detail::default_impls
    {
        static constexpr auto complete = // NOLINT
            []<class Index, class State, class Rcvr, class Tag, class... Args>(
                Index, State &state, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void
            requires(
                not std::same_as<Tag, set_value_t> ||
                std::is_invocable_v<decltype(state.template get<2>()),
                                    decltype(auto(state.template get<1>())), Args &...>)
        {
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                auto &[policy, shape, f] = state;
                constexpr bool nothrow = noexcept(f(auto(shape), args...)); // NOLINT
                if constexpr (nothrow)
                {
                    [&]() noexcept(nothrow) {
                        for (decltype(auto(shape)) i = 0; i < shape; ++i)
                        {
                            f(auto(i), args...);
                        }
                        Tag()(std::move(rcvr), std::forward<Args>(args)...);
                    }();
                }
                else
                {
                    try
                    {
                        [&]() noexcept(nothrow) {
                            for (decltype(auto(shape)) i = 0; i < shape; ++i)
                            {
                                f(auto(i), args...);
                            }
                            Tag()(std::move(rcvr), std::forward<Args>(args)...);
                        }();
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

    template <>
    struct snd::general::impls_for<adapt::bulk_chunked_t> : snd::__detail::default_impls
    {
        static constexpr auto complete = // NOLINT
            []<class Index, class State, class Rcvr, class Tag, class... Args>(
                Index, State &state, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void
            requires(
                not std::same_as<Tag, set_value_t> ||
                std::is_invocable_v<decltype(state.template get<2>()),
                                    decltype(auto(state.template get<1>())),
                                    decltype(auto(state.template get<1>())), Args &...>)
        {
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                auto &[policy, shape, f] = state; // NOLINTNEXTLINE
                constexpr bool nothrow = noexcept(f(auto(shape), auto(shape), args...));
                if constexpr (nothrow)
                {
                    using Shape = decltype(auto(shape));
                    [&]() noexcept(nothrow) {
                        f(Shape{0}, auto(shape), args...);
                        Tag()(std::move(rcvr), std::forward<Args>(args)...);
                    }();
                }
                else
                {
                    try
                    {
                        using Shape = decltype(auto(shape));
                        [&]() noexcept(nothrow) {
                            f(Shape{0}, auto(shape), args...);
                            Tag()(std::move(rcvr), std::forward<Args>(args)...);
                        }();
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

    template <>
    struct snd::general::impls_for<adapt::bulk_t> : snd::__detail::default_impls
    {
        // Note: args is a pack of lvalue subexpressions referring to the value completion
        // Note: result datums of the input sender,otherwise undefined behavior.
        static constexpr auto complete = // NOLINT
            []<class Index, class State, class Rcvr, class Tag, class... Args>(
                Index, State &state, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void
            requires(
                not std::same_as<Tag, set_value_t> ||
                std::is_invocable_v<decltype(state.template get<2>()),
                                    decltype(auto(state.template get<1>())), Args &...>)
        {
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                auto &[policy, shape, f] = state;
                constexpr bool nothrow = noexcept(f(auto(shape), args...)); // NOLINT
                auto new_f = [func = std::move(f)]<typename Size, typename... Vs>(
                                 Size begin, Size end, Vs &...vs) noexcept(nothrow) {
                    while (begin != end)
                        func(begin++, std::forward<Vs &>(vs)...);
                };
                // bucause State &state , &state need lvalue
                auto new_state =
                    snd::__detail::product_type{policy, shape, std::move(new_f)};
                impls_for<adapt::bulk_chunked_t>::complete(
                    Index(), new_state, rcvr, Tag(), std::forward<Args>(args)...);
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    template <typename Sndr, typename Policy, typename Shape, typename Fun,
              typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<
            adapt::bulk_t, snd::__detail::product_type<Policy, Shape, Fun>, Sndr>,
        Env...>
    {
        static constexpr auto is_nothorw = [] consteval { // NOLINT
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto nothorw = []<class Tag, class... As>(Tag (*)(As...)) {
                if constexpr (std::same_as<Tag, set_value_t>)
                    return std::is_nothrow_invocable_v<Fun, Shape, As &...>;
                else
                    return true;
            };
            auto no_throw_all =
                [&]<class... Sig>(cmplsigs::completion_signatures<Sig...>) {
                    return (nothorw(static_cast<Sig *>(nullptr)) && ...);
                };
            return no_throw_all(CS{});
        };

        using type = decltype(snd::completion_signatures_of_t<Sndr, Env...>{} +
                              cmplsigs::eptr_completion_if<is_nothorw()>);
    };
    template <typename Sndr, typename Policy, typename Shape, typename Fun,
              typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<
            adapt::bulk_chunked_t, snd::__detail::product_type<Policy, Shape, Fun>, Sndr>,
        Env...>
    {
        static constexpr auto is_nothorw = [] consteval { // NOLINT
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto nothorw = []<class Tag, class... As>(Tag (*)(As...)) {
                if constexpr (std::same_as<Tag, set_value_t>)
                    return std::is_nothrow_invocable_v<Fun, Shape, Shape, As &...>;
                else
                    return true;
            };
            auto no_throw_all =
                [&]<class... Sig>(cmplsigs::completion_signatures<Sig...>) {
                    return (nothorw(static_cast<Sig *>(nullptr)) && ...);
                };
            return no_throw_all(CS{});
        };

        using type = decltype(snd::completion_signatures_of_t<Sndr, Env...>{} +
                              cmplsigs::eptr_completion_if<is_nothorw()>);
    };
    template <typename Sndr, typename Policy, typename Shape, typename Fun,
              typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::bulk_unchunked_t,
                                    snd::__detail::product_type<Policy, Shape, Fun>,
                                    Sndr>,
        Env...>
    {
        static constexpr auto is_nothorw = [] consteval { // NOLINT
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto nothorw = []<class Tag, class... As>(Tag (*)(As...)) {
                if constexpr (std::same_as<Tag, set_value_t>)
                    return std::is_nothrow_invocable_v<Fun, Shape, As &...>;
                else
                    return true;
            };
            auto no_throw_all =
                [&]<class... Sig>(cmplsigs::completion_signatures<Sig...>) {
                    return (nothorw(static_cast<Sig *>(nullptr)) && ...);
                };
            return no_throw_all(CS{});
        };

        using type = decltype(snd::completion_signatures_of_t<Sndr, Env...>{} +
                              cmplsigs::eptr_completion_if<is_nothorw()>);
    };

    namespace diagnostics
    {
        template <typename Sndr, typename Policy, typename Shape, typename Fun,
                  typename... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<
                adapt::bulk_t, snd::__detail::product_type<Policy, Shape, Fun>, Sndr>,
            Env...> = []() consteval {
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto fn = []<class... As>(set_value_t (*)(As...)) {
                if constexpr (!std::invocable<Fun, Shape, As &...>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::bulk_t), WITH_SENDER(Sndr), WITH_FUNCTION(Fun),
                        WITH_ARGUMENTS(Shape, As & ...), WITH_ENV(Env...),
                        NOTE_INFO(
                            The_previous_completion_signature_does_not_match_the_current_function)>();
            };
            CS::check_sigs(overload_set{fn, [](auto) {
                                        }});
            return true;
        }();

        template <typename Sndr, typename Policy, typename Shape, typename Fun,
                  typename... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<adapt::bulk_chunked_t,
                                        snd::__detail::product_type<Policy, Shape, Fun>,
                                        Sndr>,
            Env...> = []() consteval {
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto fn = []<class... As>(set_value_t (*)(As...)) {
                if constexpr (!std::invocable<Fun, Shape, Shape, As &...>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::bulk_chunked_t), WITH_SENDER(Sndr),
                        WITH_FUNCTION(Fun), WITH_ARGUMENTS(Shape, Shape, As & ...),
                        WITH_ENV(Env...),
                        NOTE_INFO(
                            The_previous_completion_signature_does_not_match_the_current_function)>();
            };
            CS::check_sigs(overload_set{fn, [](auto) {
                                        }});
            return true;
        }();

        template <typename Sndr, typename Policy, typename Shape, typename Fun,
                  typename... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<adapt::bulk_unchunked_t,
                                        snd::__detail::product_type<Policy, Shape, Fun>,
                                        Sndr>,
            Env...> = []() consteval {
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto fn = []<class... As>(set_value_t (*)(As...)) {
                if constexpr (!std::invocable<Fun, Shape, As &...>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::bulk_unchunked_t), WITH_SENDER(Sndr),
                        WITH_FUNCTION(Fun), WITH_ARGUMENTS(Shape, As & ...),
                        WITH_ENV(Env...),
                        NOTE_INFO(
                            The_previous_completion_signature_does_not_match_the_current_function)>();
            };
            CS::check_sigs(overload_set{fn, [](auto) {
                                        }});
            return true;
        }();

    }; // namespace diagnostics

}; // namespace mcs::execution