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

            template <movable_value E>
            auto operator()(E &&e) const
                -> pipeable::sender_adaptor<stopped_as_error_t, E>
            {
                return {*this, std::forward<E>(e)};
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
        inline constexpr stopped_as_error_t stopped_as_error{}; // NOLINT
    }; // namespace adapt

    template <typename Sndr, typename E, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::stopped_as_error_t, E, Sndr>, Env>
    {
        using Add_E = cmplsigs::completion_signatures<set_error_t(E),
                                                      set_error_t(std::exception_ptr)>;
        using Filter_Sig = cmplsigs::__detail::filter_sigs_by_completion<
            set_stopped_t, snd::completion_signatures_of_t<Sndr, Env>>;

        using type = typename tfxcmplsigs::unique_variadic_template<
            typename cmplsigs::__detail::merge_type_lists<cmplsigs::completion_signatures,
                                                          Filter_Sig, Add_E>::type>::type;
    };

}; // namespace mcs::execution