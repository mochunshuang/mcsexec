#pragma once
#include <utility>
#include <variant>
#include <tuple>

#include "../snd/__sender.hpp"
#include "../snd/__completion_signatures_of_t.hpp"
#include "../snd/__sender_for.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__JOIN_ENV.hpp"
#include "../snd/general/__SCHED_ENV.hpp"
#include "../snd/general/__FWD_ENV.hpp"
#include "../snd/general/__emplace_from.hpp"
#include "../snd/general/__MAKE_ENV.hpp"
#include "../snd/general/__get_domain_early.hpp"

#include "../snd/__detail/mate_type/__child_type.hpp"

#include "../queries/__get_domain.hpp"
#include "../queries/__get_completion_scheduler.hpp"
#include "../queries/__get_env.hpp"
#include "../queries/__env_of_t.hpp"

#include "../cmplsigs/__detail/__select_tag.hpp"
#include "../cmplsigs/__detail/__filter_tuple.hpp"
#include "../cmplsigs/__detail/__tpl_param_trnsfr.hpp"
#include "../cmplsigs/__detail/__filter_sigs_by_completion.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../conn/__connect_result_t.hpp"

#include "../opstate/__start.hpp"

namespace mcs::execution
{
    namespace adapt
    {

        template <typename Completion>
        struct let_env_t
        {

            template <snd::sender Sndr>
            auto operator()(const Sndr &sndr) const // noexcept
            {
                // functional::decayed_typeof<recv::set_value> is Completion respectively
                if constexpr (requires() {
                                  snd::general::SCHED_ENV(
                                      queries::get_completion_scheduler<Completion>(
                                          queries::get_env(sndr)));
                              })
                {
                    return snd::general::SCHED_ENV(
                        queries::get_completion_scheduler<Completion>(
                            queries::get_env(sndr)));
                }
                else if constexpr (requires() {
                                       snd::general::MAKE_ENV(
                                           queries::get_domain,
                                           queries::get_domain(queries::get_env(sndr)));
                                   })
                {
                    return snd::general::MAKE_ENV(
                        queries::get_domain, queries::get_domain(queries::get_env(sndr)));
                }
                else
                {
                    return (void(sndr), empty_env{});
                }
            }
        };

        //[exec.let]
        // let_value, let_error, and let_stopped transform a sender’s value, error,
        // and stopped completions respectively into a new child asynchronous
        // operation by passing the sender’s result datums to a user-specified
        // callable, which returns a new sender that is connected and started.
        template <typename Completion>
        struct __let_t
        {
            using __tag_t = __let_t;

            template <snd::sender Sndr, movable_value Fun>
            auto operator()(Sndr &&sndr, Fun &&f) const
            {
                return snd::transform_sender(snd::general::get_domain_early(sndr),
                                             snd::make_sender(*this, std::forward<Fun>(f),
                                                              std::forward<Sndr>(sndr)));
            }
            template <snd::sender Sndr, typename Env>
                requires(snd::sender_for<Sndr, __let_t>)
            auto transform_env(Sndr &&sndr, Env &&env) noexcept // NOLINT
            {
                auto l_env = adapt::let_env_t<Completion>{};
                return snd::general::JOIN_ENV(
                    l_env(sndr), snd::general::FWD_ENV(std::forward<Env>(env)));
            }
        };

        using let_value_t = __let_t<set_value_t>;
        using let_error_t = __let_t<set_error_t>;
        using let_stopped_t = __let_t<set_stopped_t>;

        inline constexpr let_value_t let_value{};     // NOLINT
        inline constexpr let_error_t let_error{};     // NOLINT
        inline constexpr let_stopped_t let_stopped{}; // NOLINT

    }; // namespace adapt

    namespace adapt
    {
        template <class Rcvr, class Env>
        struct receiver2
        {
            using receiver_concept = receiver_t;

            template <class... Args>
            void set_value(Args &&...args) && noexcept // NOLINT
            {
                recv::set_value(std::move(rcvr), std::forward<Args>(args)...);
            }

            template <class Error>
            void set_error(Error &&err) && noexcept // NOLINT
            {
                recv::set_error(std::move(rcvr), std::forward<Error>(err));
            }

            void set_stopped() && noexcept // NOLINT
            {
                recv::set_stopped(std::move(rcvr));
            }

            decltype(auto) get_env() const noexcept // NOLINT
            {
                return snd::general::JOIN_ENV(
                    env, snd::general::FWD_ENV(queries::get_env(rcvr)));
            }

            Rcvr &rcvr; // exposition only // NOLINT
            Env env;    // exposition only // NOLINT
        };

        ////////////////////////////////////////////////////////////////
        // compute_args_variant_t
        namespace __detail
        {
            template <typename _Sig>
            struct as_tuple_no_tag;
            template <typename Tag, typename... Args>
            struct as_tuple_no_tag<Tag(Args...)>
            {
                using type = ::mcs::execution::decayed_tuple<Args...>;
            };
        }; // namespace __detail

        template <typename Sigs>
        struct compute_args_variant_t;
        template <typename... Sig>
        struct compute_args_variant_t<std::tuple<Sig...>>
        {
            using type = typename tfxcmplsigs::unique_variadic_template<std::variant<
                std::monostate, typename __detail::as_tuple_no_tag<Sig>::type...>>::type;
        };

        ////////////////////////////////////////////////////////////////
        // compute_ops2_variant_t
        namespace __detail
        {
            template <typename Fn, typename... Args>
            using as_sndr2 = functional::call_result_t<Fn, std::decay_t<Args> &...>;

            template <typename Fn, typename Sig, typename Rcvr, typename Env>
            struct compute_connect_result_t;

            template <typename Fn, typename Rcvr, typename Env, typename Tag,
                      typename... Args>
                requires(requires() { typename as_sndr2<Fn, Args...>; })
            struct compute_connect_result_t<Fn, Tag(Args...), Rcvr, Env>
            {
                using type = conn::connect_result_t<as_sndr2<Fn, Args...>,
                                                    adapt::receiver2<Rcvr, Env>>;
            };
        }; // namespace __detail

        template <typename Fn, typename Completion, typename Rcvr, typename Env>
        struct compute_ops2_variant_t;

        template <typename Fn, typename Rcvr, typename Env, typename... Sig>
        struct compute_ops2_variant_t<Fn, std::tuple<Sig...>, Rcvr, Env>
        {
            using type = typename tfxcmplsigs::unique_variadic_template<
                std::variant<std::monostate, typename __detail::compute_connect_result_t<
                                                 Fn, Sig, Rcvr, Env>::type...>>::type;
        };

        ////////////////////////////////////////////////////////////////
        // let_bind
        // Note: set_complete call this function
        template <class State, class Rcvr, class... Args>
        void let_bind(State &state, Rcvr &rcvr, Args &&...args)
        {
            using args_t = decayed_tuple<Args...>; // std::tuple
            auto mkop2 = [&] {
                // Note: sender returned by f and create a completion
                return conn::connect(
                    std::apply(std::move(state.fn), state.args.template emplace<args_t>(
                                                        std::forward<Args>(args)...)),
                    adapt::receiver2{rcvr, std::move(state.env)});
            };
            // Note: Requirement 2: makes this completion dependent on the mkop2
            // Note: call (mkop2()).start(), this completion done when mkop2 completion
            opstate::start(state.ops2.template emplace<decltype(mkop2())>(
                snd::general::emplace_from{mkop2}));
        }

    }; // namespace adapt

    template <typename Completion>
    struct snd::general::impls_for<adapt::__let_t<Completion>>
        : snd::__detail::default_impls
    {
        // initialized with a callable object
        // Note: used by general::impls_for<tag_of_t<Sndr>>::get_state
        // Note: used by basic_state, by connect(sndr,recr)
        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr, Rcvr & /*rcvr*/)
            requires(requires() {
                typename adapt::compute_args_variant_t<
                    typename cmplsigs::__detail::filter_tuple<
                        cmplsigs::__detail::select_tag<Completion>::template predicate,
                        cmplsigs::__detail::tpl_param_trnsfr_t<
                            std::tuple, snd::completion_signatures_of_t<
                                            snd::__detail::mate_type::child_type<Sndr>,
                                            queries::env_of_t<Rcvr>>>>::type>::type;
                typename adapt::compute_ops2_variant_t<
                    decltype(sndr.apply(
                        [](auto &, auto &fn, auto & /*child*/) { return fn; })),
                    typename cmplsigs::__detail::filter_tuple<
                        cmplsigs::__detail::select_tag<Completion>::template predicate,
                        cmplsigs::__detail::tpl_param_trnsfr_t<
                            std::tuple, snd::completion_signatures_of_t<
                                            snd::__detail::mate_type::child_type<Sndr>,
                                            queries::env_of_t<Rcvr>>>>::type,
                    Rcvr,
                    decltype(adapt::let_env_t<Completion>{}(
                        sndr.apply([](auto &, auto & /*fn*/, auto &child) {
                            return child;
                        })))>::type;
            })
        {
            return sndr.apply([](auto &, auto &fn, auto &child) {
                constexpr auto let_env = adapt::let_env_t<Completion>(); // NOLINT

                using Fn = std::decay_t<decltype(fn)>;
                using Env = decltype(let_env(child));

                // Sigs is type : completion_signatures<completion(As...)...>
                using Sigs = snd::completion_signatures_of_t<
                    snd::__detail::mate_type::child_type<Sndr>, queries::env_of_t<Rcvr>>;

                // std::tuple<Sig...>,the sig tag is Completion
                using LetSigs = typename cmplsigs::__detail::filter_tuple<
                    cmplsigs::__detail::select_tag<Completion>::template predicate,
                    cmplsigs::__detail::tpl_param_trnsfr_t<std::tuple, Sigs>>::type;

                // Let as-tuple be an alias template such that as-tuple<Tag(Args...)>
                // denotes the type decayed-tuple<Args...>. Then args_variant_t denotes
                // the type variant<monostate, as-tuple<LetSigs>...> except with duplicate
                // types removed.
                using args_variant_t =
                    typename adapt::compute_args_variant_t<LetSigs>::type;

                // Varint<monostate,connect_result_t<as_sndr2,Reciver2>...>,
                // and as_sndr2 is result_t by call fun with ags...
                // and as_sndr2 is Sndr
                using ops2_variant_t =
                    typename adapt::compute_ops2_variant_t<Fn, LetSigs, Rcvr, Env>::type;
                struct state_type
                {
                    Fn fn;               // exposition only
                    Env env;             // exposition only
                    args_variant_t args; // exposition only
                    ops2_variant_t ops2; // exposition only
                };
                return state_type{std::forward_like<Sndr>(fn), let_env(child), {}, {}};
            });
        };

        // initialized with a callable object
        // Note: for let-cpo(sndr, f),the rcvr of the sndr call this complete with args...
        // Note: 0. auto p = connect(top_sender,sync_recr)
        // Note: 1. connect(top_sender,sync_recr) => basic_operation (default)
        // Note: 2. opstate::start(op); // below is default behavior: call inner start()
        //  void start() & noexcept // basic_operation default behavior
        //  {
        //     inner_ops.apply([&]<typename... Op>(Op &...ops) {
        //         general::impls_for<tag_t>::start(this->state, this->rcvr, ops...);
        //     });
        //  }
        // Note: 3. state.loop.run() => recv::set_*(move(o)) (basic_recv.set_*)
        //      and basic_recv.set_* => call complete(...) => complete call nest
        // Note: for detail see "__sync_wait.hpp"
        static constexpr auto complete = // NOLINT
            []<class Index, class Tag, class... Args>(Index, auto &state, auto &rcvr, Tag,
                                                      Args &&...args) noexcept -> void {
            if constexpr (std::same_as<Tag, Completion>)
            {
                // TRY_EVAL(rcvr, let_bind(state, rcvr, std::forward<Args>(args)...));
                try
                {
                    // Note: Requirement 1: Note: below f exist in parameter state
                    //  invokes f when set-cpo is called with sndr's result datums
                    // Note: Requirement 2: Note；sender returned by f
                    //  makes its completion dependent on
                    //  the completion of a sender returned by f
                    adapt::let_bind(state, rcvr, std::forward<Args>(args)...);
                }
                catch (...)
                {
                    recv::set_error(std::move(rcvr), std::current_exception());
                }
            }
            else
            {
                // Note: Requirement 3:
                //  propagates the other completion operations sent by sndr.
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    // completion_signatures_for_impl
    namespace adapt
    {
        template <typename Fun, typename Sig>
        struct compute_fun_result;

        template <typename Fun, typename Completion, typename... Sig>
            requires(requires() { typename functional::call_result_t<Fun, Sig...>; })
        struct compute_fun_result<Fun,
                                  cmplsigs::completion_signatures<Completion(Sig...)>>
        {
            using type = functional::call_result_t<Fun, Sig...>;
        };

        // sndr的send参数列表，调用fun。获得new_sndr。使用new_sndr的签名做签名
        template <typename Fun, typename Completion, typename Sig>
        struct compute_let_sigs;
        template <typename Fun, typename Completion, typename... Sig>
        struct compute_let_sigs<Fun, Completion, cmplsigs::completion_signatures<Sig...>>
        {
            using type = cmplsigs::__detail::filter_sigs_by_completion<
                Completion, cmplsigs::completion_signatures<Sig...>>;
        };

    }; // namespace adapt

    template <typename Completion, typename Fun, typename Sender, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::__let_t<Completion>, Fun, Sender>, Env>
    {
        using type = snd::completion_signatures_of_t<
            typename adapt::compute_fun_result<
                Fun, typename adapt::compute_let_sigs<
                         Fun, Completion,
                         snd::completion_signatures_of_t<Sender, Env>>::type>::type,
            Env>;
    };

}; // namespace mcs::execution