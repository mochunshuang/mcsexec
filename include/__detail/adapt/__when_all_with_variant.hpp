#pragma once

#include "./__when_alll.hpp"
#include "./__into_variant.hpp"
#include <utility>

namespace mcs::execution
{
    namespace adapt
    {
        struct when_all_with_variant_t
        {
            template <snd::sender... Sndrs>
            auto operator()(Sndrs &&...sndrs) const // noexcept
                requires(sizeof...(Sndrs) != 0 &&
                         static_cast<bool>((snd::sender<Sndrs> && ...)) && requires() {
                             typename std::common_type_t<
                                 decltype(snd::general::get_domain_early(
                                     std::as_const(sndrs)))...>;
                         })
            {
                using CD = std::common_type_t<decltype(snd::general::get_domain_early(
                    std::as_const(sndrs)))...>;
                return snd::transform_sender(
                    CD(), snd::make_sender(*this, {}, std::forward<Sndrs>(sndrs)...));
            }

            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&sndr, const Env &env) noexcept
                requires(snd::sender_for<decltype((sndr)), when_all_with_variant_t>)
            {
                // auto &&[_, _, ...child] = sndr;
                // return when_all(
                //     into_variant(std::forward_like<decltype((sndr))>(child))...);
                return std::forward<Sndr>(sndr).apply(
                    []<typename... Child>(auto &&, auto &&, Child &&...child) {
                        return when_all(into_variant(std::forward_like<Sndr>(child))...);
                    });
            }
        };
        inline constexpr when_all_with_variant_t when_all_with_variant{}; // NOLINT
    }; // namespace adapt

    template <typename Env, typename... Sndr>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::when_all_with_variant_t, snd::empty_data,
                                    Sndr...>,
        Env>
    {
        using type = cmplsigs::completion_signatures_for_impl<
            snd::__detail::basic_sender<
                adapt::when_all_t, snd::empty_data,
                decltype(std::declval<decltype(adapt::into_variant(
                             std::declval<Sndr>()))>())...>,
            Env>::type;
    };

}; // namespace mcs::execution