#pragma once

#include "../__core_types.hpp"

#include "../snd/__detail/__product_type.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__impls_for.hpp"

namespace mcs::execution
{
    namespace factories
    {
        namespace __detail
        {
            template <typename Completion, typename... T>
            concept just_completion =
                std::same_as<Completion, set_value_t> ||
                (std::same_as<Completion, set_error_t> && (1U == sizeof...(T))) ||
                (std::same_as<Completion, set_stopped_t> && (0U == sizeof...(T)));

        }; // namespace __detail

        template <typename Completion>
        struct __just_t
        {
            template <movable_value... Ts>
                requires __detail::just_completion<Completion, Ts...>
            constexpr auto operator()(Ts &&...ts) const noexcept
            {
                return snd::make_sender(
                    *this, snd::__detail::product_type{std::forward<Ts>(ts)...});
            }
        };

        using just_t = __just_t<set_value_t>;
        using just_error_t = __just_t<set_error_t>;
        using just_stopped_t = __just_t<set_stopped_t>;

        inline constexpr just_t just{};                 // NOLINT
        inline constexpr just_error_t just_error{};     // NOLINT
        inline constexpr just_stopped_t just_stopped{}; // NOLINT
    }; // namespace factories

    template <typename Completion>
    struct snd::general::impls_for<factories::__just_t<Completion>>
        : snd::__detail::default_impls
    {
        static constexpr auto start = // NOLINT
            []<class State>(State &state, auto &rcvr) noexcept -> void {
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                Completion()(std::move(rcvr), std::move(state.template get<I>())...);
            }(std::make_index_sequence<State::size()>{});
        };
    };

    template <typename Completion, typename Env, typename... T>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<factories::__just_t<Completion>,
                                    snd::__detail::product_type<T...>>,
        Env>
    {
        using type = cmplsigs::completion_signatures<Completion(T...)>;
    };

}; // namespace mcs::execution
