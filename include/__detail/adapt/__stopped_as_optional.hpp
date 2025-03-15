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

            using type =
                decltype(tfxcmplsigs::transform_completion_signatures<
                         Input_Sig, Add_Sig, Map_V_Sig, Map_E_Sig, Ensure_No_stop>());
        };
    }; // namespace adapt::__detail

    template <typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::stopped_as_optional_t, snd::empty_data, Sndr>,
        Env...>
    {
        static consteval auto get_sigs() // NOLINT
        {
            using Sigs = snd::completion_signatures_of_t<Sndr, Env...>;
            using V_Sigs = decltype(Sigs::template filter_sigs<set_value_t>());
            []<class... Sig>(cmplsigs::completion_signatures<Sig...>) {
                if constexpr (sizeof...(Sig) != 1)
                    return tfxcmplsigs::invalid_completion_signature<
                        IN_TAG(adapt::stopped_as_optional_t),
                        WITH_SIG(cmplsigs::completion_signatures<Sig...>),
                        WITH_ENV(Env...),
                        NOTE_INFO(
                            only_accepts_a_senders_with_one_sig_with_tag_of_set_value_t)>();
            }(V_Sigs{});

            // std::is_nothrow_move_constructible_v<V>;
            constexpr auto value_transform = []<class... As>() { // NOLINT
                if constexpr (sizeof...(As) == 0)
                    return tfxcmplsigs::invalid_completion_signature<
                        IN_TAG(adapt::stopped_as_optional_t),
                        WITH_SIG(set_value_t(As...)),
                        NOTE_INFO(parameter_packageis_of_tag_of_set_value_t_is_void)>();
                else if constexpr (sizeof...(As) == 1)
                    return cmplsigs::completion_signatures<set_value_t(
                        std::optional<As...>)>();
                else
                    return cmplsigs::completion_signatures<set_value_t(
                        std::optional<std::tuple<As...>>)>();
            };
            constexpr auto stop_transform = cmplsigs::completion_signatures<>(); // NOLINT
            return tfxcmplsigs::transform_completion_signatures(
                Sigs{}, {}, value_transform, {}, stop_transform);
        }
        using type = decltype(get_sigs());
    };

}; // namespace mcs::execution