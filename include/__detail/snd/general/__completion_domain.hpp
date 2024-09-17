#pragma once
#include <tuple>

#include "../__default_domain.hpp"
#include "../../queries/__get_env.hpp"
#include "../../queries/__get_domain.hpp"
#include "../../queries/__get_completion_scheduler.hpp"

namespace mcs::execution::snd::general
{
    /////////////////////////////////////////////////
    // [exec.utils.sedr]
    namespace __detail
    {
        template <typename Tag, typename Sndr>
        concept COMPL_DOMAIN = requires(const Sndr &sndr) { // NOLINT
            queries::get_domain(
                queries::get_completion_scheduler<Tag>(queries::get_env(sndr)));
        };

    }; // namespace __detail

    template <class Default = default_domain, class Sndr>
    constexpr auto completion_domain(const Sndr &sndr) noexcept
    {
        if constexpr (not __detail::COMPL_DOMAIN<set_value_t, Sndr> &&
                      not __detail::COMPL_DOMAIN<set_error_t, Sndr> &&
                      not __detail::COMPL_DOMAIN<set_stopped_t, Sndr>)
        {
            return Default();
        }
        else
        {
            using queries::get_domain;
            using queries::get_completion_scheduler;
            using queries::get_env;
            struct __ignore;
            // COMPL_DOMAIN have will-fomat
            using set_v = std::conditional_t<__detail::COMPL_DOMAIN<set_value_t, Sndr>,
                                             decltype(get_domain(
                                                 get_completion_scheduler<set_value_t>(
                                                     get_env(sndr)))),
                                             __ignore>;
            using set_e = std::conditional_t<__detail::COMPL_DOMAIN<set_error_t, Sndr>,
                                             decltype(get_domain(
                                                 get_completion_scheduler<set_error_t>(
                                                     get_env(sndr)))),
                                             __ignore>;
            using set_s = std::conditional_t<__detail::COMPL_DOMAIN<set_stopped_t, Sndr>,
                                             decltype(get_domain(
                                                 get_completion_scheduler<set_stopped_t>(
                                                     get_env(sndr)))),
                                             __ignore>;

            // remove __ignore
            using R = decltype(std::tuple_cat(
                std::declval<std::conditional_t<not std::is_same_v<set_v, __ignore>,
                                                std::tuple<set_v>, std::tuple<>>>(),
                std::declval<std::conditional_t<not std::is_same_v<set_e, __ignore>,
                                                std::tuple<set_e>, std::tuple<>>>(),
                std::declval<std::conditional_t<not std::is_same_v<set_s, __ignore>,
                                                std::tuple<set_s>, std::tuple<>>>()));

            static_assert(not std::is_same_v<R, std::tuple<>>,
                          "Sndr no completion_domain!");

            // Otherwise is ill-formed. so bellow must will-formed
            using CommonType = decltype([]<std::size_t... I>(std::index_sequence<I...>) {
                return std::common_type_t<std::tuple_element_t<I, R>...>();
            }(std::make_index_sequence<std::tuple_size_v<R>>{}));
            return CommonType();
        }
    }
}; // namespace mcs::execution::snd::general
