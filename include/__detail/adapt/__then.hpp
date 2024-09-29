#pragma once

#include <functional>
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
        namespace __detail
        {
            template <typename Completion, typename Fun_Return>
            struct then_sig
            {
                using type = Completion(Fun_Return);
            };
            template <typename Completion>
            struct then_sig<Completion, void>
            {
                using type = Completion();
            };

            template <typename Fun, typename Completion, typename Sig>
            struct compute_then_result;
            template <typename Fun, typename C, typename C_Other, typename... Args>
            struct compute_then_result<Fun, C, C_Other(Args...)>
            {
                // Forwards all other completion operations unchanged
                using type = C_Other(Args...);
            };

            template <typename Fun, typename Completion, typename... Args>
            // TODO(mcs): Because the number of implicit conversion, calls may not be just
            // one if matching Completion not just one
            // Note: then 作为前一个sender的延续，理论上仅仅修改一个签名
            struct compute_then_result<Fun, Completion, Completion(Args...)>
            {
                using type =
                    typename then_sig<Completion,
                                      functional::call_result_t<Fun, Args...>>::type;
            };
        }; // namespace __detail

        template <typename Fun, typename Completion, typename Sig>
        struct compute_then_sigs;
        template <typename Fun, typename Completion, typename... Sig>
        struct compute_then_sigs<Fun, Completion, cmplsigs::completion_signatures<Sig...>>
        {
            using type = cmplsigs::completion_signatures<
                typename __detail::compute_then_result<Fun, Completion, Sig>::type...>;
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
