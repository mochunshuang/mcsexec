#pragma once

#include <tuple>
#include <variant>

#include "./__as_tuple.hpp"

#include "../snd/__sender.hpp"
#include "../snd/__sender_in.hpp"
#include "../snd/__completion_signatures_of_t.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/__transform_sender.hpp"

#include "../snd/general/__JOIN_ENV.hpp"
#include "../snd/general/__SCHED_ATTRS.hpp"
#include "../snd/general/__FWD_ENV.hpp"

#include "../snd/__detail/mate_type/__child_type.hpp"

#include "../queries/__get_domain.hpp"
#include "../queries/__get_env.hpp"
#include "../queries/__env_of_t.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"
#include "../tfxcmplsigs/__transform_completion_signatures.hpp"

#include "../conn/__connect_result_t.hpp"

#include "../opstate/__start.hpp"

#include "../sched/__schedule_result_t.hpp"

#include "../factories/__schedule.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        /**
         * @brief schedule_from schedules work dependent on the completion of a sender
         * onto a scheduler’s associated execution resource.
         */
        // Note: schedule_from is not meant to be used in user code; it is used in the
        // Note: implementation of continues_on.
        struct schedule_from_t
        {
            template <sched::scheduler Sched, snd::sender Sndr>
            auto operator()(Sched &&sch, Sndr &&sndr) const
            {
                auto dom = snd::general::query_or_default(
                    queries::get_domain, std::as_const(sch), snd::default_domain());
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Sched>(sch),
                                          std::forward<Sndr>(sndr)));
            };
        };
        inline constexpr schedule_from_t schedule_from{}; // NOLINT
    }; // namespace adapt

    namespace adapt
    {
        template <typename Sigs>
        struct compute_variant_t;
        template <typename... Sig>
        struct compute_variant_t<cmplsigs::completion_signatures<Sig...>>
        {
            using type = typename tfxcmplsigs::unique_variadic_template<std::variant<
                std::monostate, typename __detail::as_tuple<Sig>::type...>>::type;
        };

        template <typename Sigs, typename Rcvr, typename sched_t>
        struct state_type
        {
            Rcvr &rcvr; // NOLINT

            struct receiver_t
            {
                using receiver_concept = ::mcs::execution::receiver_t;

                state_type *state; // exposition only // NOLINT

                void set_value() && noexcept // NOLINT
                {
                    std::visit(
                        [this]<class Tuple>(Tuple &result) noexcept -> void {
                            if constexpr (not std::same_as<std::monostate, Tuple>)
                            {
                                std::apply(
                                    [&]<class Tag, typename... Args>(Tag &tag,
                                                                     Args &...args) {
                                        tag(std::move(state->rcvr), std::move(args)...);
                                    },
                                    result);
                            }
                        },
                        state->async_result);
                }

                template <class Error>
                void set_error(Error &&err) && noexcept // NOLINT
                {
                    recv::set_error(std::move(state->rcvr), std::forward<Error>(err));
                }

                void set_stopped() && noexcept // NOLINT
                {
                    recv::set_stopped(std::move(state->rcvr));
                }

                decltype(auto) get_env() const noexcept // NOLINT
                {
                    return snd::general::FWD_ENV(queries::get_env(state->rcvr));
                }
            };

            using variant_t = typename adapt::compute_variant_t<Sigs>::type;
            using operation_t =
                conn::connect_result_t<sched::schedule_result_t<sched_t>, receiver_t>;

            operation_t op_state;   // exposition only // NOLINT
            variant_t async_result; // exposition only // NOLINT

            explicit state_type(sched_t sch, Rcvr &rcvr) noexcept(
                noexcept(conn::connect(factories::schedule(sch), receiver_t{this})))
                : rcvr(rcvr),
                  op_state(conn::connect(factories::schedule(sch), receiver_t{this}))
            {
            }
        };

    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::schedule_from_t> : snd::__detail::default_impls
    {
        static constexpr auto get_attrs = // NOLINT
            [](const auto &data, const auto &child) noexcept -> decltype(auto) {
            return snd::general::JOIN_ENV(snd::general::SCHED_ATTRS(data),
                                          snd::general::FWD_ENV(queries::get_env(child)));
        };

        // Note: used by basic_state initialized when connect(out_sndr,out_recr)
        static constexpr auto get_state = // NOLINT
            []<class OutSndr, class OutRcvr>(OutSndr &&sndr, OutRcvr &rcvr) noexcept(true)
            requires(snd::sender_in<snd::__detail::mate_type::child_type<OutSndr>,
                                    queries::env_of_t<OutRcvr>>)
        {
            auto &[_, sch, child] = sndr;
            using sched_t = decltype(auto(sch));

            // Note: add Sigs for like just() | then that need add Sigs for sync_wait
            using Sigs = tfxcmplsigs::transform_completion_signatures<
                snd::completion_signatures_of_t<OutSndr, queries::env_of_t<OutRcvr>>,
                cmplsigs::completion_signatures<set_error_t(std::exception_ptr),
                                                set_stopped_t()>>;
            using state_type = adapt::state_type<Sigs, OutRcvr, sched_t>;

            return state_type{sch, rcvr};
        };

        static constexpr auto complete = // NOLINT
            []<class Tag, class... Args>(auto, auto &state, auto &rcvr, Tag,
                                         Args &&...args) noexcept -> void {
            using result_t = decayed_tuple<Tag, Args...>;

            constexpr bool nothrow = // NOLINT
                std::is_nothrow_constructible_v<result_t, Tag, Args...>;

            // TRY_EVAL(rcvr,exor );
            try
            {
                [&]() noexcept(nothrow) {
                    state.async_result.template emplace<result_t>(
                        Tag(), std::forward<Args>(args)...);
                }();
            }
            catch (...)
            {
                recv::set_error(std::move(rcvr), std::current_exception());
            }

            if (state.async_result.valueless_by_exception())
                return;
            if (state.async_result.index() == 0)
                return;

            opstate::start(state.op_state);
        };
    };

    template <typename Sched, typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::schedule_from_t, Sched, Sndr>, Env>
    {
        using type = tfxcmplsigs::transform_completion_signatures<
            snd::completion_signatures_of_t<Sndr, Env>,
            cmplsigs::completion_signatures<set_error_t(std::exception_ptr),
                                            set_stopped_t()>>;
    };

}; // namespace mcs::execution