#pragma once

#include <utility>
#include "./__continues_on.hpp"
#include "./__starts_on.hpp"

#include "../snd/general/__SCHED_ENV.hpp"
#include "../snd/general/__query_with_default.hpp"

#include "../queries/__get_completion_scheduler.hpp"

#include "../factories/__write_env.hpp"

#include "../snd/__not_a_sender.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        template <typename Sched, typename Sndr>
        concept on_check_one =
            not sched::scheduler<Sched> ||
            (not snd::sender<Sndr> &&
             not std::derived_from<std::decay_t<Sndr>, pipeable::sender_adaptor_closure<
                                                           std::decay_t<Sndr>>>) ||
            (snd::sender<Sndr> &&
             std::derived_from<std::decay_t<Sndr>,
                               pipeable::sender_adaptor_closure<std::decay_t<Sndr>>>);

        template <typename Sched, typename Sndr, typename Adaptor>
        concept on_check_two =
            not sched::scheduler<Sched> || not snd::sender<Sndr> ||
            not std::derived_from<std::decay_t<Adaptor>, pipeable::sender_adaptor_closure<
                                                             std::decay_t<Adaptor>>>;

        struct not_a_scheduler
        {
        };

        struct on_t
        {
            /**
             * Note: Start [sndr] on sch then callback rcvr's sch
             * Let out_sndr be on(sch, sndr)
             * Let op be an lvalue from connecting [out_sndr] with [out_rcvr]
             * Calling start(op) shall:
             * 1、Remember the current scheduler, get_scheduler(get_env(rcvr))
             * 2、Start [sndr] on the sch's associated execution resource
             * 3、Upon sndr's completion, transfer execution back to sched in step 1
             * 4、Forward sndr's async result to [out_rcvr]
             */
            template <typename Sched, typename Sndr>
                requires(not on_check_one<Sched, Sndr>)
            auto operator()(Sched &&sch, Sndr &&sndr) const noexcept
            {
                auto dom = snd::general::query_or_default(
                    queries::get_domain, std::as_const(sch), snd::default_domain());
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sched>(sch),
                                          std::forward<Sndr>(sndr)));
            }

            /**
             * Note: Start [sndr] on cur sch, Upon sndr's completion, transfer execution
             * Note: to sch's associated, then call closure with sndr's async result
             * Note: Upon closure(S) completion, transfer execution to sch in step 1
             * Note: and forward the operation’s async result to [out_rcvr]
             *
             * Let [out_sndr] be on(sndr, sch, closure)
             * Let op be an lvalue from connecting [out_sndr] with [out_rcvr]
             * Calling start(op) shall:
             * 1、Remember the current scheduler, which is the first of the following
             * expressions that is well-formed:
             *      -get_completion_scheduler<set_value_t>(get_env(sndr))
             *      -get_scheduler(get_env(rcvr))
             * 2、Start sndr on the current execution agent execution resource
             * 3、Upon sndr's completion, transfer execution to sch's associated execution
             * resource.
             * 4、Forward sndr's async result as if by connecting and starting a sender
             * closure(S), where S is a sender that completes synchronously with sndr's
             * async result.
             * 5、Upon completion of the operation started in step 4,transfer execution
             * back to the execution to execution resource in step 1, and forward the
             * operation’s async result to [out_rcvr]
             */
            template <typename Sndr, typename Sched, typename Adaptor>
                requires(not on_check_two<Sched, Sndr, Adaptor>)
            auto operator()(Sndr &&sndr, Sched &&sch, Adaptor &&closure) const noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(
                             *this,
                             snd::__detail::product_type{std::forward<Sched>(sch),
                                                         std::forward<Adaptor>(closure)},
                             std::forward<Sndr>(sndr)));
            }

            template <snd::sender Sndr, typename E>
            auto transform_env(Sndr &&out_sndr, E &&env) noexcept // NOLINT
                requires(snd::sender_for<decltype((out_sndr)), on_t>)
            {
                // Note: optimization for no copy
                using OutSndr = decltype((out_sndr));
                using Env = decltype((env));
                auto &&[_, data, __] = std::forward<Sndr>(out_sndr);
                if constexpr (sched::scheduler<decltype(data)>)
                {
                    return snd::general::JOIN_ENV(
                        snd::general::SCHED_ENV(std::forward_like<OutSndr>(data)),
                        snd::general::FWD_ENV(std::forward<Env>(env)));
                }
                else
                {
                    return std::forward<Env>(env);
                }
            }

            template <snd::sender Sndr, typename Env> // NOLINTNEXTLINE
            auto transform_sender(Sndr &&out_sndr, const Env &env) noexcept
                requires(snd::sender_for<decltype((out_sndr)), on_t>)
            {
                // Note: optimization for no copy
                using OutSndr = decltype((out_sndr));
                auto &&[_, data, child] = std::forward<Sndr>(out_sndr);

                if constexpr (sched::scheduler<decltype(data)>)
                {
                    auto orig_sch = snd::general::query_with_default(
                        queries::get_scheduler, env, not_a_scheduler());

                    if constexpr (std::same_as<decltype(orig_sch), not_a_scheduler>)
                    {
                        return not_a_sender{};
                    }
                    else
                    {
                        return continues_on(starts_on(std::forward_like<OutSndr>(data),
                                                      std::forward_like<OutSndr>(child)),
                                            std::move(orig_sch));
                    }
                }
                else
                {
                    auto &[sch, closure] = data;
                    auto orig_sch = snd::general::query_with_default(
                        queries::get_completion_scheduler<set_value_t>,
                        queries::get_env(std::as_const(child)),
                        snd::general::query_with_default(queries::get_scheduler, env,
                                                         not_a_scheduler()));

                    if constexpr (std::same_as<decltype(orig_sch), not_a_scheduler>)
                    {
                        return not_a_sender{};
                    }
                    else
                    {
                        // Note: closure(sndr)
                        return factories::write_env(
                            continues_on(std::forward_like<OutSndr>(closure)(continues_on(
                                             factories::write_env(
                                                 std::forward_like<OutSndr>(child),
                                                 snd::general::SCHED_ENV(orig_sch)),
                                             sch)),
                                         orig_sch),
                            snd::general::SCHED_ENV(sch));
                    }
                }
            }
        };

        inline constexpr on_t on{}; // NOLINT

    }; // namespace adapt

    // TODO(mcs): 2 type completion_signatures_for_impl may be
    template <typename Sched, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::on_t, Sched, Sndr>, Env...>
    {
        using type = cmplsigs::completion_signatures_for_impl<
            snd::__detail::basic_sender<adapt::starts_on_t, Sched, Sndr>, Env...>::type;
    };
    template <typename Sndr, typename Sched, typename Adaptor, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::on_t, Sndr, Sched, Adaptor>, Env...>
    {
        using orig_sch = decltype(queries::get_completion_scheduler<set_value_t>(
            queries::get_env(std::as_const(std::declval<Sndr>()))));
        using WriteEnvReturnType = decltype(factories::write_env(
            std::declval<Sndr>(),
            std::declval<decltype(snd::general::SCHED_ENV(std::declval<orig_sch>()))>()));
        using ContinuesOnReturnType = decltype(adapt::continues_on(
            std::declval<WriteEnvReturnType>(),
            std::declval<decltype(std::declval<Sched>())>()));
        using ClosureCallResultType =
            decltype(std::declval<Adaptor>()(std::declval<ContinuesOnReturnType>()));

        static_assert(snd::sender<ClosureCallResultType>,
                      "closure(cotinues_sndr) must return a sndr");

        using type = snd::completion_signatures_of_t<ClosureCallResultType, Env...>;
    };

}; // namespace mcs::execution