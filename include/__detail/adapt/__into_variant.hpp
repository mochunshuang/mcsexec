#pragma once

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"

#include "../snd/__detail/mate_type/__child_type.hpp"

#include "../queries/__env_of_t.hpp"

#include "../cmplsigs/__value_types_of_t.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../cmplsigs/__eptr_completion_if.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        struct into_variant_t
        {
            template <snd::sender Sndr>
            auto operator()(Sndr &&sndr) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, {}, std::forward<Sndr>(sndr)));
            }

            auto operator()() const -> pipeable::sender_adaptor<into_variant_t>
            {
                return {{*this}};
            }
        };
        inline constexpr into_variant_t into_variant{}; // NOLINT
        namespace __detail
        {
            template <class T, bool v>
            struct into_variant_state
            {
                using type = T;
                static constexpr bool nothrow = v; // NOLINT
            };
        }; // namespace __detail
    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::into_variant_t> : snd::__detail::default_impls
    {
        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr && /*sndr*/, Rcvr & /*rcvr*/) noexcept {
                using Sigs =
                    snd::completion_signatures_of_t<Sndr, queries::env_of_t<Rcvr>>;
                using type = cmplsigs::value_types_of_t<
                    snd::__detail::mate_type::child_type<Sndr>, execution::decayed_tuple,
                    cmplsigs::variant_or_empty, queries::env_of_t<Rcvr>>;
                constexpr bool nothrow = Sigs::template count<set_error_t> == 0; // NOLINT
                return adapt::__detail::into_variant_state<type, nothrow>{};
            };
        static constexpr auto complete = // NOLINT
            []<class State, class Rcvr, class Tag, class... Args>(
                auto, State, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void {
            // Note: just handler the tag of set_value_t
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                using variant_type = typename State::type;
                constexpr bool nothrow = State::nothrow; // NOLINT
                if constexpr (nothrow)
                {
                    if constexpr (std::is_void_v<decltype(variant_type(
                                      decayed_tuple<Args...>{
                                          std::forward<Args>(args)...}))>)
                    {
                        variant_type(decayed_tuple<Args...>{std::forward<Args>(args)...});
                        recv::set_value(std::move(rcvr));
                    }
                    else
                        recv::set_value(std::move(rcvr),
                                        variant_type(decayed_tuple<Args...>{
                                            std::forward<Args>(args)...}));
                }
                else
                {
                    try
                    {
                        if constexpr (std::is_void_v<decltype(variant_type(
                                          decayed_tuple<Args...>{
                                              std::forward<Args>(args)...}))>)
                        {
                            variant_type(
                                decayed_tuple<Args...>{std::forward<Args>(args)...});
                            recv::set_value(std::move(rcvr));
                        }
                        else
                            recv::set_value(std::move(rcvr),
                                            variant_type(decayed_tuple<Args...>{
                                                std::forward<Args>(args)...}));
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

    template <typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::into_variant_t, snd::empty_data, Sndr>, Env...>
    {
        using Sigs = snd::completion_signatures_of_t<Sndr, Env...>;
        using Sigs_s = decltype(Sigs::template filter_sigs<set_stopped_t>());
        static constexpr bool nothrow = Sigs::template count<set_error_t> == 0; // NOLINT
        static consteval auto get_all_v()                                       // NOLINT
        {
            return cmplsigs::completion_signatures<set_value_t(
                typename cmplsigs::gather_signatures<set_value_t, Sigs, decayed_tuple,
                                                     cmplsigs::variant_or_empty>)>{};
        };
        using type = decltype(get_all_v() + eptr_completion_if<nothrow> + Sigs_s{});
    };

}; // namespace mcs::execution
