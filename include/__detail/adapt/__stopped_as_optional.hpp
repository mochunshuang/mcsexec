#pragma once
#include <optional>

#include "./__let_value.hpp"
#include "./__then.hpp"

#include "../factories/__just.hpp"

#include "../snd/general/__get_domain_early.hpp"

#include "../cmplsigs/__single_sender_value_type.hpp"

#include "../tfxcmplsigs/__transform_completion_signatures.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        // stopped_as_optional maps a sender’s stopped completion operation into a value
        // completion operation as an disengaged optional.
        struct stopped_as_optional_t
        {
            template <snd::sender Sndr>
            auto operator()(Sndr &&sndr) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, {}, std::forward<Sndr>(sndr)));
            }

            auto operator()() const -> pipeable::sender_adaptor<stopped_as_optional_t>
            {
                return {*this};
            }

            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&sndr, const Env &env) noexcept
                requires(snd::sender_for<decltype((sndr)), stopped_as_optional_t> &&
                         requires() {
                             not std::is_same_v<
                                 cmplsigs::single_sender_value_type<
                                     snd::__detail::mate_type::child_type<Sndr>, Env>,
                                 void>;
                         })
            {
                auto &&[_, __, child] = sndr;
                using V = cmplsigs::single_sender_value_type<decltype(child), Env>;
                return let_stopped(
                    then(std::forward_like<Sndr>(child),
                         []<class... Ts>(Ts &&...ts) noexcept(
                             std::is_nothrow_constructible_v<V, Ts...>) {
                             return std::optional<V>(std::in_place,
                                                     std::forward<Ts>(ts)...);
                         }),
                    []() noexcept { return factories::just(std::optional<V>()); });
            }
        };
        inline constexpr stopped_as_optional_t stopped_as_optional{}; // NOLINT

    }; // namespace adapt

    namespace adapt::__detail
    {
        // Note: std::optional<void>,std::optional<int,int> is compile error
        template <typename... T>
            requires(sizeof...(T) == 1)
        using Map_V_Sig = cmplsigs::completion_signatures<set_value_t(
            std::optional<std::decay_t<T>>...)>;

        template <typename T>
        using Map_E_Sig = cmplsigs::completion_signatures<set_error_t(T)>;

        template <typename Sndr, typename Env>
        struct compute_stopped_as_optional_sigs
        {
            using Input_Sig = snd::completion_signatures_of_t<Sndr, Env>;
            using Add_Sig =
                cmplsigs::completion_signatures<set_error_t(std::exception_ptr)>;
            using Ensure_No_stop = cmplsigs::completion_signatures<>;

            using type = tfxcmplsigs::transform_completion_signatures<
                Input_Sig, Add_Sig, Map_V_Sig, Map_E_Sig, Ensure_No_stop>;
        };
    }; // namespace adapt::__detail

    template <typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::stopped_as_optional_t, snd::empty_data, Sndr>,
        Env>
    {
        using type =
            typename adapt::__detail::compute_stopped_as_optional_sigs<Sndr, Env>::type;
    };

}; // namespace mcs::execution