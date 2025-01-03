#pragma once

#include <exception>
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

#include "../tfxcmplsigs/__unique_variadic_template.hpp"
#include "../cmplsigs/__detail/__merge_type_lists.hpp"

#include "../tool/Make_Return_Sigs.hpp"
#include "../tool/Complile_Error_As_Error_Sig.hpp"

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
            // Note: using the result value of f as then-cpo(sndr, f) value completion
            if constexpr (std::same_as<Tag, Completion>)
            {
                try
                {
                    if constexpr (std::is_void_v<std::invoke_result_t<Fn, Args...>>)
                    {
                        std::invoke(std::move(fn), std::forward<Args>(args)...);
                        recv::set_value(std::move(rcvr));
                    }
                    else
                    {
                        recv::set_value(
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

        /**
         * Note: to value completion or Forward for all completion sigs
         * @brief The expression then-cpo(sndr, f) has undefined behavior unless it
         * returns a sender out_sndr that: 1、Invokes f or a copy of such with the value,
         * error, or stopped result datums of sndr for then, upon_error, and upon_stopped
         * [respectively], using the result value of f as out_sndr's value completion, and
         *
         * 2、Forwards all [other completion] operations unchanged.
         *
         * Note: must hande respectively completion anyway, and Forwards other completion
         */
        template <typename Fun, typename Completion, typename Sig>
        struct compute_then_sigs;
        template <typename Fun, typename Completion, typename... Sig>
        struct compute_then_sigs<Fun, Completion, cmplsigs::completion_signatures<Sig...>>
        {

            using Add_Sig =
                cmplsigs::completion_signatures<recv::set_error_t(std::exception_ptr)>;

            using Must_Handle_Sigs =
                typename cmplsigs::__detail::filter_sigs_by_completion<
                    Completion, cmplsigs::completion_signatures<Sig...>>::type;
            // Note: Must_Forward_Sigs include May_Forward_V_Sigs
            using Must_Forward_Sigs =
                typename cmplsigs::__detail::skip_sigs_by_completion<
                    Completion, cmplsigs::completion_signatures<Sig...>>::type;

            using Next_V_Sig = tool::Generate_V_Sigs<Fun, Must_Handle_Sigs>::type;

            static_assert(
                not tool::Complile_Error_As_Error_Sig<Fun, Completion, Next_V_Sig>::value,
                "fun_parm and pre sndr sig not match");

            using type = // Note: only handle match set_tag
                typename tfxcmplsigs::unique_variadic_template<
                    typename cmplsigs::__detail::merge_type_lists<
                        cmplsigs::completion_signatures, Next_V_Sig, Must_Forward_Sigs,
                        Add_Sig>::type>::type;
        };
    }; // namespace adapt

    template <typename Completion, typename Fun, typename Sender, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::__then_t<Completion>, Fun, Sender>, Env>
    {
        using type = typename adapt::compute_then_sigs<
            Fun, Completion, snd::completion_signatures_of_t<Sender, Env>>::type;
    };

}; // namespace mcs::execution
