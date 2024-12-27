#pragma once

#include "./__as_tuple.hpp"

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/general/__on_stop_request.hpp"
#include "../snd/__completion_signatures_of_t.hpp"
#include "../snd/__detail/mate_type/__child_type.hpp"

#include "../snd/__sender_in.hpp"

#include "../queries/__env_of_t.hpp"
#include "../queries/__stop_token_of_t.hpp"

#include "../conn/__connect_result_t.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../cmplsigs/__value_types_of_t.hpp"

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
        };
        inline constexpr into_variant_t into_variant{}; // NOLINT
    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::into_variant_t> : snd::__detail::default_impls
    {
        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr && /*sndr*/, Rcvr & /*rcvr*/) noexcept
            -> std::type_identity<cmplsigs::value_types_of_t<
                snd::__detail::mate_type::child_type<Sndr>, queries::env_of_t<Rcvr>>> {
            return {};
        };
        static constexpr auto complete = // NOLINT
            []<class State, class Rcvr, class Tag, class... Args>(
                auto, State, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void {
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                using variant_type = typename State::type;
                // TRY_SET_VALUE(rcvr, variant_type(decayed_tuple<Args...>{
                //                         std::forward<Args>(args)...}));
                try
                {
                    if constexpr (std::is_void_v<decltype(variant_type(
                                      decayed_tuple<Args...>{
                                          std::forward<Args>(args)...}))>)
                    {
                        variant_type(decayed_tuple<Args...>{std::forward<Args>(args)...});
                        recv::set_value(std::move(rcvr));
                    }
                    else
                    {
                        recv::set_value(std::move(rcvr),
                                        variant_type(decayed_tuple<Args...>{
                                            std::forward<Args>(args)...}));
                    }
                }
                catch (...)
                {
                    recv::set_error(std::move(rcvr), std::current_exception());
                }
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    template <typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::into_variant_t, Sndr>, Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

}; // namespace mcs::execution
