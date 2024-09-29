#pragma once

#include <utility>
#include "./__continues_on.hpp"
#include "./__starts_on.hpp"

#include "../snd/general/__SCHED_ENV.hpp"
#include "../snd/general/__query_with_default.hpp"

#include "../queries/__get_completion_scheduler.hpp"

#include "../factories/__write_env.hpp"

#include "../tfxcmplsigs/__transform_completion_signatures.hpp"

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

        struct not_a_sender
        {
            using sender_concept = sender_t;

            auto get_completion_signatures(auto &&) const // NOLINT
            {
                struct not_completion_signatures
                {
                };
                return not_completion_signatures{};
            }
        };
        struct not_a_scheduler
        {
        };

        struct on_t
        {
            template <typename Sched, typename Sndr>
                requires(not on_check_one<Sched, Sndr> && snd::sender<Sndr>)
            auto operator()(Sched &&sch, Sndr &&sndr) const
            {
                auto dom = snd::general::query_or_default(
                    queries::get_domain, std::as_const(sch), snd::default_domain());
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sched>(sch),
                                          std::forward<Sndr>(sndr)));
            }

            template <typename Sndr, typename Sched, typename Adaptor>
                requires(not on_check_two<Sched, Sndr, Adaptor>)
            auto operator()(Sndr &&sndr, Sched &&sch, Adaptor &&closure) const
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(
                             *this,
                             snd::__detail::product_type{std::forward<Sched>(sch),
                                                         std::forward<Adaptor>(closure)},
                             std::forward<Sndr>(sndr)));
            }

            template <snd::sender Sndr, typename Env>
            auto transform_env(Sndr &&out_sndr, Env &&env) noexcept // NOLINT
                requires(snd::sender_for<decltype((out_sndr)), on_t>)
            {
                // Note: optimization for no copy
                using OutSndr = decltype(out_sndr);
                auto &&[_, data, __] = out_sndr;
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
                using OutSndr = decltype(out_sndr);
                auto &&[_, data, child] = out_sndr;

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

    template <typename Sched, typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::on_t, Sched, Sndr>, Env>
    {
        // using type = snd::completion_signatures_of_t<Sndr, Env>;
        using type = tfxcmplsigs::transform_completion_signatures<
            snd::completion_signatures_of_t<Sndr, Env>,
            snd::completion_signatures_of_t<decltype(std::declval<Sched>().schedule()),
                                            Env>>;
    };

}; // namespace mcs::execution