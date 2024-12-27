#pragma once

#include "./__let_value.hpp"
#include "../factories/__just.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        // stopped_as_error maps an input sender’s stopped completion operation into an
        // error completion operation as a custom error type.
        struct stopped_as_error_t
        {
            template <snd::sender Sndr, movable_value Err>
            auto operator()(Sndr &&sndr, Err &&err) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Err>(err),
                                          std::forward<Sndr>(sndr)));
            }
            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&sndr, const Env &env) noexcept
                requires(snd::sender_for<decltype((sndr)), stopped_as_error_t>)
            {
                auto &&[_, err, child] = sndr;
                using E = decltype(auto(err));
                return let_stopped(
                    std::forward_like<Sndr>(child),
                    [err = std::forward_like<Sndr>(err)]() mutable noexcept(
                        std::is_nothrow_move_constructible_v<E>) {
                        return factories::just_error(std::move(err));
                    });
            }
        };
    }; // namespace adapt

    template <typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::stopped_as_error_t, Sndr>, Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

}; // namespace mcs::execution