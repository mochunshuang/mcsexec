#pragma once

#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__JOIN_ENV.hpp"
#include "../snd/general/__MAKE_ENV.hpp"
#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/general/__on_stop_request.hpp"

#include "../snd/__sender_in.hpp"

#include "../queries/__env_of_t.hpp"
#include "../queries/__stop_token_of_t.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../cmplsigs/__value_types_of_t.hpp"
#include "../cmplsigs/__error_types_of_t.hpp"
#include "../cmplsigs/__detail/__concat_tuples.hpp"

#include "../__stoptoken/__stop_callback_of_t.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        struct when_all_t
        {
            template <snd::sender... Sndrs>
            auto operator()(Sndrs &&...sndrs) const // noexcept
                requires(sizeof...(Sndrs) != 0 &&
                         static_cast<bool>((snd::sender<Sndrs> && ...)) && requires() {
                             typename std::common_type_t<
                                 decltype(snd::general::get_domain_early(sndrs))...>;
                         })
            {
                using CD = std::common_type_t<decltype(snd::general::get_domain_early(
                    sndrs))...>;
                return snd::transform_sender(
                    CD(), snd::make_sender(*this, {}, std::forward<Sndrs>(sndrs)...));
            }
        };

        inline constexpr when_all_t when_all{}; // NOLINT
    }; // namespace adapt

    namespace adapt::__when_all
    {
        template <class Sndr, class Env>
        concept max_1_sender_in = // exposition only
            snd::sender_in<Sndr, Env> &&
            (std::tuple_size_v<
                 cmplsigs::value_types_of_t<Sndr, Env, std::tuple, std::tuple>> <= 1);

        enum class disposition
        {
            started, // NOLINT
            error,   // NOLINT
            stopped  // NOLINT
        }; // exposition only
        struct none_such
        {
        };

        namespace __detail
        {
            template <typename To, typename From>
            struct strans_args;
            template <typename To, typename... T>
            struct strans_args<To, std::tuple<T...>>
            {
                using type = To(T...);
            };

            template <>
            struct strans_args<set_value_t, std::tuple<>>
            {
                using type = set_value_t();
            };
            template <>
            struct strans_args<set_error_t, std::tuple<>>
            {
                // Note: set_error_t()是非法的
                using type = set_error_t(std::exception_ptr);
                // using type = set_error_t(none_such);
            };

            /**
             * @brief 工具类。计算简单单个Sndr发送的值类型
             */
            template <typename T>
            struct Sndr_return_type;

            // Note: 普通的单个Sndr，应该只能有一个值通道；因此不用形参包
            template <typename T>
            struct Sndr_return_type<std::tuple<T>>
            {
                using type = T;
            };
            template <>
            struct Sndr_return_type<std::tuple<>>
            {
                using type = void;
            };

            template <typename... T>
            struct All_Sndr_value_types;

            template <typename... Tuple>
            struct All_Sndr_value_types<std::variant<Tuple>...>
            {
                struct ignore;
                using original_t = decltype(std::tuple_cat(
                    std::declval<std::conditional_t<
                        not std::is_void_v<typename Sndr_return_type<Tuple>::type>,
                        std::tuple<typename Sndr_return_type<Tuple>::type>,
                        std::tuple<>>>()...));
                // using type = set_value_t(typename Sndr_return_type<Tuple>::type...);
                // static_assert(std::is_same_v<original_t, std::tuple<>>);
                using type = typename strans_args<set_value_t, original_t>::type;
            };

        }; // namespace __detail
        namespace __detail
        {
            ///////////////////////////////////////////////////////////////
            /////////// compute_values_tuple
            template <typename Rcvr, typename... Sndrs>
            struct compute_values_tuple;

            template <typename Rcvr, typename... Sndrs>
                requires(requires() {
                    typename std::tuple<cmplsigs::value_types_of_t<
                        Sndrs, queries::env_of_t<Rcvr>, decayed_tuple, std::optional>...>;
                })
            struct compute_values_tuple<Rcvr, Sndrs...>
            {
                using type = std::tuple<cmplsigs::value_types_of_t<
                    Sndrs, queries::env_of_t<Rcvr>, decayed_tuple, std::optional>...>;
            };
            template <typename Rcvr, typename... Sndrs>
                requires(not requires() {
                    typename std::tuple<cmplsigs::value_types_of_t<
                        Sndrs, queries::env_of_t<Rcvr>, decayed_tuple, std::optional>...>;
                })
            struct compute_values_tuple<Rcvr, Sndrs...>
            {
                using type = std::tuple<>;
            };

            ///////////////////////////////////////////////////////////////
            /////////// compute_errors_variant
            namespace __detail
            {
                template <typename T>
                auto decay_copy(T &&value) noexcept(
                    noexcept(std::decay_t<T>(std::forward<T>(value)))) -> std::decay_t<T>
                {
                    return std::forward<T>(value);
                }

                template <typename... Ts>
                concept is_decay_copy_noexcept_v =
                    (noexcept(decay_copy(std::declval<Ts>())) && ...);

            }; // namespace __detail

            /**
             * @brief Let copy-fail be exception_ptr if decay-copying any of the child
             * senders' result datums can potentially throw; otherwise, none-such, where
             * none-such is an unspecified empty class type.
             *
             * @tparam Es
             */
            template <typename... V>
            using copy_fail = std::conditional_t<__detail::is_decay_copy_noexcept_v<V...>,
                                                 none_such, std::exception_ptr>;

            template <typename Env, typename... Sndrs>
            struct compute_copy_fail_exception_tuple;

            template <typename T>
            struct compute_copy_fail_exception_tuple_impl;

            template <typename... T>
            struct compute_copy_fail_exception_tuple_impl<set_value_t(T...)>
            {
                using type = copy_fail<T...>;
            };

            template <typename Env, typename... Sndrs>
            struct compute_copy_fail_exception_tuple
            {
                using Type = std::conditional_t<
                    std::is_same_v<
                        typename compute_copy_fail_exception_tuple_impl<
                            typename All_Sndr_value_types<
                                cmplsigs::value_types_of_t<Sndrs, Env>...>::type>::type,
                        none_such>,
                    std::tuple<>, std::tuple<std::exception_ptr>>;
            };

            template <typename Tuple>
            struct compute_errors_variant_impl;
            template <typename... Es>
            struct compute_errors_variant_impl<std::tuple<Es...>>
            {
                using type = tfxcmplsigs::unique_variadic_template<
                    std::variant<none_such, std::decay_t<Es>...>>::type;
            };

            template <typename Sndrs>
            struct compute_es;

            template <typename Env, typename... Sndrs>
            struct compute_errors_variant;

            template <typename Env, typename... Sndrs>
            struct compute_errors_variant
            {
                // where Es is the pack of the decayed types of all the child senders'
                // possible error result datums.
                // Note: compute: turple<Es>....，frist then concat_tuples it to one tuple
                using type = typename compute_errors_variant_impl<
                    typename tfxcmplsigs::unique_variadic_template<
                        typename cmplsigs::__detail::concat_tuples<
                            typename compute_copy_fail_exception_tuple<Env,
                                                                       Sndrs...>::Type,
                            std::conditional_t<
                                std::is_same_v<
                                    cmplsigs::error_types_of_t<Sndrs, Env, std::tuple>,
                                    cmplsigs::empty_variant>,
                                std::tuple<>,
                                cmplsigs::error_types_of_t<Sndrs, Env, std::tuple>>...>::
                            type>::type>::type;
            };

        }; // namespace __detail

        template <class Rcvr>
        struct make_state
        {
            template <max_1_sender_in<queries::env_of_t<Rcvr>>... Sndrs>
            auto operator()(auto /*unused*/, auto /*unused*/, Sndrs &&...sndrs) const
            {
                /**
                 * @brief The alias values_tuple denotes the type
                 * tuple<value_types_of_t<Sndrs, env_of_t<Rcvr>, decayed-tuple,
                 * optional>...> if that type is well-formed; otherwise, tuple<>.
                 *
                 */
                using values_tuple =
                    typename __detail::compute_values_tuple<Rcvr, Sndrs...>::type;
                /**
                 * @brief The alias errors_variant denotes the type
                 *  variant<none-such,copy-fail, Es...> with duplicate types removed,
                 *  where Es is the pack of the decayed types of all the child senders'
                 *  possible error result datums.
                 */
                // variant 用 none-such 做哨兵类型
                using errors_variant =
                    typename __detail::compute_errors_variant<queries::env_of_t<Rcvr>,
                                                              Sndrs...>::type;
                // stop_callback == token + CallbackFn
                using stop_callback = typename stoptoken::stop_callback_of_t<
                    queries::stop_token_of_t<queries::env_of_t<Rcvr>>,
                    snd::general::on_stop_request>;

                struct state_type
                {
                    void arrive(Rcvr &rcvr) noexcept
                    {
                        if (0 == --count)
                        {
                            this->complete(rcvr);
                        }
                    }

                    std::atomic<size_t> count{sizeof...(sndrs)};         // NOLINT
                    inplace_stop_source stop_src{};                      // NOLINT
                    std::atomic<disposition> disp{disposition::started}; // NOLINT
                    errors_variant errors{};                             // NOLINT
                    values_tuple values{};                               // NOLINT
                    std::optional<stop_callback> on_stop{std::nullopt};  // NOLINT

                    // Note: 3路完成：value,error,stop的赋值
                    void complete(Rcvr &rcvr) noexcept
                    {
                        // 1、 If disp is equal to disposition::started, evaluates:
                        if (disp.load() == disposition::started)
                        {
                            auto tie = []<class... T>(std::tuple<T...> &t) noexcept {
                                return std::tuple<T &...>(t);
                            };
                            auto set = [&](auto &...t) noexcept {
                                recv::set_value(std::move(rcvr), std::move(t)...);
                            };
                            this->on_stop.reset();
                            std::apply(
                                [&](auto &...opts) noexcept {
                                    std::apply(set, std::tuple_cat(tie(*opts)...));
                                },
                                values);
                        }
                        // 2、 Otherwise, if disp is equal to disposition::error,
                        // evaluates:
                        else if (disp.load() == disposition::error)
                        {
                            this->on_stop.reset();
                            std::visit(
                                [&]<class Error>(Error &error) noexcept {
                                    if constexpr (not std::same_as<Error, none_such>)
                                    {
                                        recv::set_error(std::move(rcvr),
                                                        std::move(error));
                                    }
                                },
                                errors);
                        }
                        // 3. Otherwise
                        else
                        {
                            this->on_stop.reset();
                            recv::set_stopped(std::move(rcvr));
                        }
                    }
                };

                return state_type{};
            };
        };
    }; // namespace adapt::__when_all

    template <>
    struct snd::general::impls_for<adapt::when_all_t> : snd::__detail::default_impls
    {
        static constexpr auto get_attrs = // NOLINT
            [](auto &&, auto &&...child) noexcept {
                using CD = std::common_type_t<decltype(snd::general::get_domain_early(
                    child))...>;
                if constexpr (std::same_as<CD, default_domain>)
                {
                    return empty_env();
                }
                else
                {
                    return snd::general::MAKE_ENV(queries::get_domain, CD());
                }
            };

        static constexpr auto get_env = // NOLINT
            []<class State, class Rcvr>(auto &&, State &state,
                                        const Rcvr &rcvr) noexcept {
                return snd::general::JOIN_ENV(
                    snd::general::MAKE_ENV(queries::get_stop_token,
                                           state.stop_src.get_token()),
                    queries::get_env(rcvr));
            };

        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr, Rcvr & /*rcvr*/) noexcept(
                // std::forward<Sndr>(std::declval<Sndr>())
                //     .apply(adapt::__when_all::make_state<Rcvr>())
                true) -> auto {
            return std::forward<Sndr>(sndr).apply(adapt::__when_all::make_state<Rcvr>());
        };

        static constexpr auto start = // NOLINT
            []<class State, class Rcvr, class... Ops>(State &state, Rcvr &rcvr,
                                                      Ops &...ops) noexcept -> void {
            // Note: 赋值 make_state::on_stop.值为 token + stop_callback的组合模板实例
            // 其中：on_stop_request 是 stop_callback。当stop_request发生时调用
            // Note: 标准规定，request_stop之后的 注册的callback，都立即调用
            state.on_stop.emplace(queries::get_stop_token(queries::get_env(rcvr)),
                                  snd::general::on_stop_request{state.stop_src});
            if (state.stop_src.stop_requested())
            {
                // Note: 收到 request_stop 后，不再start. 立即有异步结果： stopped通道
                state.on_stop.reset();
                recv::set_stopped(std::move(rcvr));
            }
            else
            {
                (opstate::start(ops), ...);
            }
        };

        /**
         * @brief
         *  Note: 该算法由 Base_receiver调用：recv::set_xxx
         *  complete(Index(), parent_op->state, parent_op->rcvr, tag(),tag_args...);
         *  tag 有 3 路，tag_args 映射对应的 tag
         */
        static constexpr auto complete = // NOLINT
            []<class Index, class State, class Rcvr, class Set, class... Args>(
                this auto &complete, Index, State &state, Rcvr &rcvr, Set,
                Args &&...args) noexcept -> void {
            // 1、 set_error 通道
            if constexpr (std::same_as<Set, set_error_t>)
            {
                if (adapt::__when_all::disposition::error !=
                    state.disp.exchange(adapt::__when_all::disposition::error))
                {
                    state.stop_src.request_stop();
                    // Note: TRY_EMPLACE_ERROR(v, e);
                    // Note: TRY_EMPLACE_ERROR(state.errors, std::forward<Args>(args)...);
                    constexpr bool no_throw = // NOLINT
                        (noexcept(decltype(auto(std::forward<Args>(args)))(
                             std::forward<Args>(args))) &&
                         ...);
                    if constexpr (not no_throw)
                    {
                        try
                        {
                            state.errors.template emplace<decltype(auto(
                                std::forward<Args>(args)))...>(
                                std::forward<Args>(args)...);
                        }
                        catch (...)
                        {
                            state.errors.template emplace<std::exception_ptr>(
                                std::current_exception());
                        }
                    }
                    else
                    {
                        state.errors.template emplace<decltype(auto(
                            std::forward<Args>(args)))...>(std::forward<Args>(args)...);
                    }
                }
            }
            // 2、 set_stopped 通道
            else if constexpr (std::same_as<Set, set_stopped_t>)
            {
                auto expected = adapt::__when_all::disposition::started;
                if (state.disp.compare_exchange_strong(
                        expected, adapt::__when_all::disposition::stopped))
                {
                    state.stop_src.request_stop();
                }
            }
            // 3、 set_value 通道
            // Note: values_tuple 不是 std::tuple<>类型。 则需要填写结果值
            else if constexpr (not std::same_as<decltype(State::values), std::tuple<>>)
            {
                if (state.disp.load() == adapt::__when_all::disposition::started)
                {
                    auto &opt = std::get<Index::value>(state.values);
                    // Note: TRY-EMPLACE-VALUE(c, o, as...)
                    // TRY_EMPLACE_VALUE(complete, opt,std::forward<Args>(args)...);
                    constexpr bool no_throw = // NOLINT
                        noexcept(decayed_tuple<decltype(std::forward<Args>(args))...>{
                            std::forward<Args>(args)...});
                    if constexpr (not no_throw)
                    {
                        try
                        {
                            opt.emplace(std::forward<Args>(args)...);
                        }
                        catch (...)
                        {
                            // Note: 递归 lambda: set_value => set_error;
                            // return作用: count 由 set_value迁移到set_error才 count--
                            // Note: count 不会多运算 --
                            complete(Index(), state, rcvr, set_error,
                                     std::current_exception());
                            return;
                        }
                    }
                    else
                    {
                        opt.emplace(std::forward<Args>(args)...);
                    }
                }
            }
            // 4、effective only when (0 == --count) == true
            // Note: 调用 make_state 的 arrive 方法。计数count--
            state.arrive(rcvr);
        };
    };

    namespace adapt::__when_all
    {
        using __detail::strans_args;
        using __detail::All_Sndr_value_types;
        using __detail::compute_copy_fail_exception_tuple;

        // std::variant<std::tuple<int, std::basic_string<char>>>
        // Note: set_value_t(int, std::string) 。
        // 两个sndr，一个返回int，一个返回string
        template <typename Env, typename... Sndr>
        struct conpute_value_types
        {
            using type = typename All_Sndr_value_types<
                cmplsigs::value_types_of_t<Sndr, Env>...>::type;
        };

        template <typename Env, typename... Sndrs>
        struct conpute_error_types
        {
            using original_type = typename tfxcmplsigs::unique_variadic_template<
                typename cmplsigs::__detail::concat_tuples<
                    typename compute_copy_fail_exception_tuple<Env, Sndrs...>::Type,
                    std::conditional_t<
                        std::is_same_v<cmplsigs::error_types_of_t<Sndrs, Env, std::tuple>,
                                       cmplsigs::empty_variant>,
                        std::tuple<>,
                        cmplsigs::error_types_of_t<Sndrs, Env, std::tuple>>...>::type>::
                type;

            using type = strans_args<set_error_t, original_type>::type;
        };

    }; // namespace adapt::__when_all

    // TODO(mcs): completion_signatures_for_impl
    // Note:  make_state() 返回  state_type
    // Note: snd::general::impls_for<adapt::when_all_t>::get_state 的结果值解析即可
    // Note: 还是独立的好，Sndr...传来。解析值类型，错误类型；组合 +
    // 当前算法而外类型即可
    template <typename Env, typename... Sndr>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::when_all_t, snd::empty_data, Sndr...>, Env>
    {
        using type = cmplsigs::completion_signatures<
            typename adapt::__when_all::conpute_value_types<Env, Sndr...>::type,
            typename adapt::__when_all::conpute_error_types<Env, Sndr...>::type>;
    };
}; // namespace mcs::execution