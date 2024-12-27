#pragma once
#include <optional>

#include "./__let_value.hpp"
#include "./__then.hpp"

#include "../factories/__just.hpp"

#include "../snd/general/__get_domain_early.hpp"

#include "../cmplsigs/__single_sender_value_type.hpp"

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

            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&sndr, const Env &env) noexcept
                requires(snd::sender_for<decltype((sndr)), stopped_as_optional_t> &&
                         requires() {
                             typename cmplsigs::single_sender_value_type<Sndr, Env>;
                             requires not std::is_same_v<
                                 cmplsigs::single_sender_value_type<Sndr, Env>, void>;
                         })
            {
                auto &&[_, __, child] = sndr;
                using V = cmplsigs::single_sender_value_type<Sndr, Env>;
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

    template <typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::stopped_as_optional_t, Sndr>, Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

}; // namespace mcs::execution