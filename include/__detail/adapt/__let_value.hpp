#pragma once
#include <concepts>
#include <exception>
#include <optional>
#include <type_traits>
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

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../conn/__connect_result_t.hpp"

#include "../opstate/__start.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../cmplsigs/__eptr_completion_if.hpp"
#include "../diagnostics/__check.hpp"

namespace mcs::execution
{
    namespace adapt
    {

        template <typename Completion>
        struct let_env_t
        {

            template <snd::sender Sndr>
            auto operator()(const Sndr &sndr) const noexcept
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

            template <snd::sender Sndr, movable_value Fun>
                requires(diagnostics::check_type<snd::__detail::basic_sender<
                             adapt::__let_t<Completion>, std::decay_t<Fun>,
                             std::decay_t<Sndr>>>)
            auto operator()(Sndr &&sndr, Fun &&f) const noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(dom,
                                             snd::make_sender(*this, std::forward<Fun>(f),
                                                              std::forward<Sndr>(sndr)));
            }

            template <movable_value Fun>
            auto operator()(Fun &&fun) const noexcept
                -> pipeable::sender_adaptor<__let_t, Fun>
            {
                return {*this, std::forward<Fun>(fun)};
            }

            template <snd::sender Sndr, typename Env>
                requires(snd::sender_for<Sndr, __let_t>)
            auto transform_env(Sndr &&sndr, Env &&env) noexcept // NOLINT
            {
                using E = adapt::let_env_t<Completion>;
                // TODO(mcs): JOIN_ENV 低效见 p3396r0
                return snd::general::JOIN_ENV(
                    E{}(std::as_const(sndr)),
                    snd::general::FWD_ENV(std::forward<Env>(env)));
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
        template <typename Env, typename Rcvr>
        struct receiver2_env_t
        {

            template <typename Q>
            constexpr auto query(Q &&q) const noexcept
            {
                if constexpr (requires { env.query(std::forward<Q>(q)); })
                    return env.query(std::forward<Q>(q));
                else if constexpr (requires {
                                       queries::get_env(rcvr).query(std::forward<Q>(q));
                                   })
                    return queries::get_env(rcvr).query(std::forward<Q>(q));
            }
            const Env &env;   // NOLINT
            const Rcvr &rcvr; // NOLINT
        };

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
                // return snd::general::JOIN_ENV(
                //     env, snd::general::FWD_ENV(queries::get_env(std::as_const(rcvr))));
                return receiver2_env_t{env, rcvr};
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

        ////////////////////////////////////////////////////////////////
        // compute_ops2_variant_t
        namespace __detail
        {
            template <typename Fn, typename... Args>
            using as_sndr2 = functional::call_result_t<Fn, std::decay_t<Args> &...>;
        }; // namespace __detail

        ////////////////////////////////////////////////////////////////
        // let_bind
        // Note: set_complete call this function
        template <bool is_nothrow, class State, class Rcvr, class... Args>
        void let_bind(State &state, Rcvr &rcvr, Args &&...args) noexcept(is_nothrow)
        {
            using args_t = decayed_tuple<Args...>; // std::tuple
            auto mkop2 = [&] noexcept(is_nothrow) {
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
        template <bool is_nothrow, typename Fn, typename Env, typename args_variant_t,
                  typename ops2_variant_t>
        struct let_state_type
        {
            static constexpr bool nothrow = is_nothrow; // NOLINT
            Fn fn;                                      // exposition only
            Env env;                                    // exposition only
            args_variant_t args;                        // exposition only
            ops2_variant_t ops2;                        // exposition only
        };
        template <typename Sigs>
        static consteval auto complete_let_args_variant()
        {
            auto fun = []<class... Sig>(cmplsigs::completion_signatures<Sig...>) {
                if constexpr (requires {
                                  typename tfxcmplsigs::unique_variadic_template<
                                      std::variant<std::monostate,
                                                   typename __detail::as_tuple_no_tag<
                                                       Sig>::type...>>::type;
                              })
                    return typename tfxcmplsigs::unique_variadic_template<std::variant<
                        std::monostate,
                        typename __detail::as_tuple_no_tag<Sig>::type...>>::type{};
                else
                    return diagnostics::invalid_completion_signature<
                        // IN_FUNCTION<complete_let_args_variant>,
                        NOTE_INFO(Failed_to_complete_signature_calculation)>();
            };
            return fun(Sigs());
        }
        // NOTE: 指针不需要 可以默认初始化默认构造
        template <typename Fn, typename Sigs, typename Rcvr, typename Env>
        static consteval auto compute_ops2_variant()
        {
            // 计算单个 completion signature 的结果
            auto compute_connect_result = []<typename Tag, typename... Args>(
                                              Tag (*)(Args...)) {
                if constexpr (requires { typename __detail::as_sndr2<Fn, Args...>; })
                {
                    using Sender = typename __detail::as_sndr2<Fn, Args...>;
                    using Result =
                        conn::connect_result_t<Sender, adapt::receiver2<Rcvr, Env>>;
                    return static_cast<Result *>(nullptr);
                }
                else
                {
                    return diagnostics::invalid_completion_signature<
                        IN_TAG(let_value_t), WITH_FUNCTION(Fn), WITH_SIG(Tag(Args...)),
                        NOTE_INFO(Failed_to_complete_signature_calculation),
                        not_satisfied_with_the_requirements()>();
                }
            };

            // 计算所有 completion signatures 的结果
            auto compute_all = []<typename... Sig>(
                                   cmplsigs::completion_signatures<Sig...>) {
                using VariantType = typename tfxcmplsigs::unique_variadic_template<
                    std::variant<std::monostate,
                                 std::remove_pointer_t<decltype(compute_connect_result(
                                     static_cast<Sig *>(nullptr)))>...>>::type;
                return static_cast<VariantType *>(nullptr);
            };
            return compute_all(Sigs{});
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
            []<class Sndr, class Rcvr>(Sndr &&sndr, Rcvr & /*rcvr*/) noexcept {
                auto &[_, fn, child] = sndr;
                constexpr auto let_env = adapt::let_env_t<Completion>(); // NOLINT

                using Fn = std::decay_t<decltype(fn)>;
                using Env = decltype(let_env(child));

                // Note: 1 : completion_signatures specialization
                //  Let Sigs be a pack of the arguments to the completion_signatures
                //  specialization named by completion_signatures_of_t<child-type<Sndr>,
                //  env_of_t<Rcvr>>
                using Sigs = snd::completion_signatures_of_t<
                    snd::__detail::mate_type::child_type<Sndr>, queries::env_of_t<Rcvr>>;

                // Note: 2 : let set-cpo be set_value, set_error, and set_stopped
                // respectively
                //  Let LetSigs be a pack of those types in [Sigs] with a return type of
                //  decayed-typeof<set-cpo>.
                using Origin_LetSigs = decltype(Sigs::template filter_sigs<Completion>());
                constexpr auto is_nothrow = // NOLINT
                    []<class... Sigs>(cmplsigs::completion_signatures<Sigs...>) {
                        if constexpr (sizeof...(Sigs) > 0)
                        {
                            auto fun = []<class... As>(Completion (*)(As...)) noexcept {
                                return noexcept(
                                    std::declval<Fn>()(std::declval<As>()...));
                            };
                            return (fun(static_cast<Sigs *>(nullptr)) && ...);
                        }
                        else
                            return true;
                    };
                constexpr bool nothrow = is_nothrow(Origin_LetSigs{}); // NOLINT
                // using add_Sig_when_set_error_t =
                //     std::conditional_t<std::is_same_v<Completion, set_error_t>,
                //                        decltype(cmplsigs::eptr_completion_if<nothrow>),
                //                        cmplsigs::completion_signatures<>>;

                using LetSigs = decltype(Origin_LetSigs{});

                // Note: 3:
                // Let as-tuple be an alias template such that as-tuple<Tag(Args...)>
                // denotes the type decayed-tuple<Args...> Note: 4 : Then
                // args_variant_t denotes the type
                // variant<monostate,as-tuple<LetSigs>...>
                using args_variant_t =
                    decltype(adapt::complete_let_args_variant<LetSigs>());

                // Varint<monostate,connect_result_t<as_sndr2,Reciver2>...>,
                // and as_sndr2 is result_t by call fun with ags...
                // and as_sndr2 is Sndr
                using ops2_variant_t = std::remove_pointer_t<
                    decltype(adapt::compute_ops2_variant<Fn, LetSigs, Rcvr, Env>())>;
                return adapt::let_state_type<nothrow, Fn, Env, args_variant_t,
                                             ops2_variant_t>{
                    std::forward_like<Sndr>(fn), let_env(child), {}, {}};
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
                constexpr auto nothrow = // NOLINT
                    std::remove_cvref_t<decltype(state)>::nothrow;
                if constexpr (nothrow)
                {
                    adapt::let_bind<nothrow>(state, rcvr, std::forward<Args>(args)...);
                }
                else
                {
                    try
                    {
                        // Note: Requirement 1: Note: below f exist in parameter state
                        //  invokes f when set-cpo is called with sndr's result datums
                        // Note: Requirement 2: Note；sender returned by f
                        //  makes its completion dependent on
                        //  the completion of a sender returned by f
                        adapt::let_bind<nothrow>(state, rcvr,
                                                 std::forward<Args>(args)...);
                    }
                    catch (...)
                    {
                        recv::set_error(std::move(rcvr), std::current_exception());
                    }
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

    /**
     * Note: 调用 `set-cpo` 并传入 `sndr` 的结果数据时，调用 `f`
     * Note: 完成依赖于 `f` 返回的发送器的完成
     * Note: 传播由 `sndr` 发送的其他完成操作。
     * Let the subexpression out_sndr denote the result of the invocation let-cpo(sndr, f)
     * or an object equal to such。
     * The expression connect(out_sndr, rcvr) has undefined behavior unless it creates an
     * asynchronous operation ([async.ops]) that, when started:
     *
     * 1、invokes f when set-cpo is called with [sndr's result datums]
     * 2、makes its completion [dependent on] the completion of a sender returned by f,and
     * 3、andpropagates the [other completion] operations sent by sndr.
     *
     */
    template <typename Completion, typename Fun, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::__let_t<Completion>, Fun, Sndr>, Env...>
    {
        static constexpr auto transform = // NOLINT
            []<class Tag, class... As>(Tag (*)(As...)) {
                if constexpr (std::is_same_v<Tag, Completion>)
                {
                    using Ret = decltype(std::declval<Fun>()(std::declval<As>()...));
                    constexpr bool nothrow = // NOLINT
                        noexcept(std::declval<Fun>()(std::declval<As>()...));
                    return snd::completion_signatures_of_t<Ret>{} +
                           eptr_completion_if<nothrow>;
                }
                else
                    return cmplsigs::completion_signatures<Tag(As...)>{};
            };
        using type =
            decltype(snd::completion_signatures_of_t<Sndr, Env...>::transform_sigs(
                transform));
    };

    namespace diagnostics
    {
        template <typename Completion, typename Fun, typename Sndr,
                  typename... Env> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<adapt::__let_t<Completion>, Fun, Sndr>,
            Env...> = []() consteval {
            using CS = snd::completion_signatures_of_t<Sndr, Env...>;
            auto fn = []<class... Ts>(Completion (*)(Ts...)) {
                if constexpr (!std::invocable<Fun, Ts...>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::__let_t<Completion>), WITH_SENDER(Sndr),
                        WITH_FUNCTION(Fun), WITH_ARGUMENTS(Ts...),
                        NOTE_INFO(
                            The_previous_completion_signature_does_not_match_the_current_function)>();
                else if constexpr (!snd::sender<std::invoke_result_t<Fun, Ts...>>)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::__let_t<Completion>), WITH_FUNCTION(Fun),
                        WITH_ARGUMENTS(Ts...),
                        NOTE_INFO(the_fun_return_type_is_not_a_sndr_in_let_xxx)>();
            };
            CS::check_sigs(overload_set{fn, [](auto) {
                                        }});
            return true;
        }();

    }; // namespace diagnostics

}; // namespace mcs::execution