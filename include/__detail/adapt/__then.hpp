#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../recv/__set_error.hpp"
#include "../recv/__set_stopped.hpp"
#include "../recv/__set_value.hpp"

#include "../cmplsigs/__detail/__filter_sigs_by_completion.hpp"
#include "../traits/__trait_function.hpp"
#include "../cmplsigs/__detail/__build_sig_from_args.hpp"
#include "../snd/general/__CONVERTIBLE_SIG.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"
#include "../cmplsigs/__detail/__merge_type_lists.hpp"
namespace mcs::execution
{
    namespace adapt
    {
        template <typename Completion>
        struct __then_t
        {
            // make_sender provides tag_of_t will-format
            template <snd::sender Sndr, movable_value Fun>
            auto operator()(Sndr &&sndr, Fun &&f) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(dom,
                                             snd::make_sender(*this, std::forward<Fun>(f),
                                                              std::forward<Sndr>(sndr)));
            }

            template <movable_value Fun>
            auto operator()(Fun &&fun) const -> pipeable::sender_adaptor<__then_t, Fun>
            {
                return {*this, std::forward<Fun>(fun)};
            }
        };

        using then_t = __then_t<recv::set_value_t>;
        using upon_error_t = __then_t<recv::set_error_t>;
        using upon_stopped_t = __then_t<recv::set_stopped_t>;

        inline constexpr then_t then{};                 // NOLINT
        inline constexpr upon_error_t upon_error{};     // NOLINT
        inline constexpr upon_stopped_t upon_stopped{}; // NOLINT

    }; // namespace adapt

    template <typename Completion>
    struct snd::general::impls_for<adapt::__then_t<Completion>>
        : snd::__detail::default_impls
    {
        static constexpr auto complete = // NOLINT
            []<class Fn, class Tag, class... Args>(auto, Fn &fn, auto &rcvr, Tag,
                                                   Args &&...args) noexcept -> void {
            // Note: only handle same Completion or forward
            if constexpr (std::same_as<Tag, Completion>)
            {
                try
                {
                    if constexpr (std::is_void_v<std::invoke_result_t<Fn, Args...>>)
                    {
                        std::invoke(std::move(fn), std::forward<Args>(args)...);
                        Completion()(std::move(rcvr));
                    }
                    else
                    {
                        Completion()(
                            std::move(rcvr),
                            std::invoke(std::move(fn), std::forward<Args>(args)...));
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

    namespace adapt
    {
        template <typename Completion, typename Ret>
        struct helper
        {
            using type = cmplsigs::completion_signatures<Completion(Ret)>;
        };

        template <typename Completion, typename Ret>
            requires std::is_void_v<Ret>
        struct helper<Completion, Ret>
        {
            using type = cmplsigs::completion_signatures<Completion()>;
        };

        template <typename Fun, typename Completion, typename Sig>
        struct compute_then_sigs;
        template <typename Fun, typename Completion, typename... Sig>
        struct compute_then_sigs<Fun, Completion, cmplsigs::completion_signatures<Sig...>>
        {

            using F_INFO = traits::trait_function<Fun>;
            using V_Sig =
                typename helper<recv::set_value_t, typename F_INFO::ret_t>::type;
            using Base_Sig =
                cmplsigs::completion_signatures<recv::set_error_t(std::exception_ptr),
                                                recv::set_stopped_t()>;
            using To =
                cmplsigs::__detail::build_sig_from_args<Completion,
                                                        typename F_INFO::arg_t>::type;
            using Pre_Sndr_Sig_list = cmplsigs::completion_signatures<Sig...>;
            static_assert(snd::general::HAS_CONVERTIBLE_SIG<Pre_Sndr_Sig_list, To>);

            using type = // Note: only handle match set_tag
                typename tfxcmplsigs::unique_variadic_template<
                    typename cmplsigs::__detail::merge_type_lists<
                        cmplsigs::completion_signatures, V_Sig, Base_Sig>::type>::type;
        };
    }; // namespace adapt

    template <typename Completion, typename Fun, typename Sender, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::__then_t<Completion>, Fun, Sender>, Env>
    {
        using type = typename adapt::compute_then_sigs<
            Fun, Completion,
            typename cmplsigs::__detail::filter_sigs_by_completion<
                Completion, snd::completion_signatures_of_t<Sender, Env>>::type>::type;
    };

}; // namespace mcs::execution
