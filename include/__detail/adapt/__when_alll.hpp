#pragma once

#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__MAKE_ENV.hpp"
#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"

#include "../queries/__env_of_t.hpp"
#include "../queries/__stop_token_of_t.hpp"

#include "../cmplsigs/__value_types_of_t.hpp"

#include "../__stoptoken/__stop_callback_of_t.hpp"

#include "../__stoptoken/__finite_inplace_stop_source.hpp"
#include "../diagnostics/__check.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        struct when_all_t
        {
            template <snd::sender... Sndrs>
                requires(sizeof...(Sndrs) != 0 &&
                         diagnostics ::check_type<snd::__detail::basic_sender<
                             adapt::when_all_t, snd::empty_data, std::decay_t<Sndrs>...>>)
            auto operator()(Sndrs &&...sndrs) const noexcept
            {

                using CD2 = decltype([]() {
                    if constexpr (requires() {
                                      typename std::common_type_t<
                                          decltype(snd::general::get_domain_early(
                                              sndrs))...>;
                                  })
                    {
                        using CD =
                            std::common_type_t<decltype(snd::general::get_domain_early(
                                sndrs))...>;
                        return CD{};
                    }
                    else
                        return snd::default_domain{};
                });
                return snd::transform_sender(
                    CD2(), snd::make_sender(*this, {}, std::forward<Sndrs>(sndrs)...));
            }
        };

        inline constexpr when_all_t when_all{}; // NOLINT
    }; // namespace adapt

    namespace adapt::__when_all
    {
        enum class disposition : std::uint8_t
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

            template <typename Source, typename Env, std::size_t Idx>
            struct when_all_env_t
            {
                constexpr auto query(
                    const queries::get_stop_token_t & /*q*/) const noexcept
                {
                    return stop_src.template get_token<Idx>();
                }
                template <typename Q>
                constexpr auto query(Q &&q) const noexcept
                    requires(requires(Env env) { env.query(std::forward<Q>(q)); })
                {
                    return env.query(std::forward<Q>(q));
                }
                const Source &stop_src; // NOLINT
                Env env;                // NOLINT
            };

        }; // namespace __detail

        template <class Rcvr>
        struct make_state
        {
            template <class... Sndrs>
            static consteval auto get_values_tuple() // NOLINT
            {
                if constexpr (requires {
                                  typename std::tuple<cmplsigs::value_types_of_t<
                                      Sndrs, decayed_tuple, std::optional,
                                      queries::env_of_t<Rcvr>>...>;
                              })
                {
                    return std::tuple<
                        cmplsigs::value_types_of_t<Sndrs, decayed_tuple, std::optional,
                                                   queries::env_of_t<Rcvr>>...>{};
                }
                else
                    return std::tuple<>{};
            };
            template <class... Sndrs>
            static consteval auto get_errors_variant() // NOLINT
            {
                using Sigs = decltype((
                    snd::completion_signatures_of_t<Sndrs, queries::env_of_t<Rcvr>>{} +
                    ... + cmplsigs::completion_signatures<>{}));
                using All_V = decltype(Sigs::template filter_sigs<set_value_t>());
                using All_E = decltype(Sigs::template filter_sigs<set_error_t>());
                constexpr auto copy_fail_all = // NOLINT
                    []<class... Sig>(cmplsigs::completion_signatures<Sig...>) consteval {
                        constexpr auto copy_fail = // NOLINT
                            []<class... As>(set_value_t (*)(As...)) {
                                if constexpr (sizeof...(As) == 0)
                                    return false;
                                else
                                    return __when_all::__detail::__detail::
                                        is_decay_copy_noexcept_v<As...>;
                            };
                        return (copy_fail(static_cast<Sig *>(nullptr)) && ...);
                    };
                constexpr bool is_copy_fail = copy_fail_all(All_V{}); // NOLINT

                if constexpr (std::is_same_v<All_E, cmplsigs::completion_signatures<>>)
                {
                    return std::conditional_t<is_copy_fail,
                                              std::variant<none_such, std::exception_ptr>,
                                              std::variant<none_such>>{};
                }
                else
                {
                    if constexpr (is_copy_fail)
                    {

                        constexpr bool contains_eptr = // NOLINT
                            All_E::template contains<set_error_t(std::exception_ptr)>;
                        if constexpr (contains_eptr)
                            return
                                []<class... Es>(
                                    cmplsigs::completion_signatures<set_error_t(Es)...>) {
                                    return std::variant<none_such, Es...>{};
                                }(All_E{});
                        else
                            return
                                []<class... Es>(
                                    cmplsigs::completion_signatures<set_error_t(Es)...>) {
                                    return std::variant<none_such, std::exception_ptr,
                                                        Es...>{};
                                }(All_E{});
                    }
                    else
                        return []<class... Es>(
                                   cmplsigs::completion_signatures<set_error_t(Es)...>) {
                            return std::variant<none_such, Es...>{};
                        }(All_E{});
                }
            };

            template <class... Sndrs>
            constexpr auto operator()(auto /*unused*/, auto /*unused*/,
                                      Sndrs &&.../*sndrs*/) const noexcept
            {
                /**
                 * @brief The alias values_tuple denotes the type
                 * tuple<value_types_of_t<Sndrs, env_of_t<Rcvr>, decayed-tuple,
                 * optional>...> if that type is well-formed; otherwise, tuple<>.
                 *
                 */

                using values_tuple = decltype(get_values_tuple<Sndrs...>());
                /**
                 * @brief The alias errors_variant denotes the type
                 *  variant<none-such,copy-fail, Es...> with duplicate types removed,
                 *  where Es is the pack of the decayed types of all the child
                 * senders' possible error result datums.
                 */
                // variant 用 none-such 做哨兵类型
                using errors_variant = decltype(get_errors_variant<Sndrs...>());

                // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3409r1.html#orgab1aee9
                using stop_source =
                    stoptoken::finite_inplace_stop_source<sizeof...(Sndrs)>;
                struct forward_stop_request
                {
                    stop_source &stop_src; // NOLINT
                    void operator()() noexcept
                    {
                        stop_src.request_stop();
                    }
                };
                // stop_callback == token + CallbackFn
                using stop_callback = typename stoptoken::stop_callback_of_t<
                    queries::stop_token_of_t<queries::env_of_t<Rcvr>>,
                    forward_stop_request>;

                struct state_type
                {
                    void arrive(Rcvr &rcvr) noexcept
                    {
                        if (0 == --count)
                        {
                            this->complete(rcvr);
                        }
                    }
                    void register_stop_callback( // exposition only // NOLINT
                        queries::stop_token_of_t<queries::env_of_t<Rcvr>> st) noexcept
                    {
                        on_stop.emplace(std::move(st), forward_stop_request{stop_src});
                    }

                    std::atomic<size_t> count{sizeof...(Sndrs)};         // NOLINT
                    stop_source stop_src{};                              // NOLINT
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

        template <std::size_t Idx, class Source, class Env>
        static constexpr auto make_when_all_env(const Source &stop_src, // NOLINT
                                                Env &&env) noexcept
        {
            return adapt::__when_all::__detail::when_all_env_t<Source, Env, Idx>{
                stop_src, std::forward<Env>(env)};
        }
        static constexpr auto get_env = // NOLINT
            []<class State, class Rcvr, std::size_t Idx>(
                std::integral_constant<size_t, Idx>, State &state,
                const Rcvr &rcvr) noexcept {
                return make_when_all_env<Idx>(state.stop_src, queries::get_env(rcvr));
            };

        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr, Rcvr & /*rcvr*/) noexcept(
                noexcept(std::forward<Sndr>(std::declval<Sndr>())
                             .apply(adapt::__when_all::make_state<Rcvr>()))) -> auto {
            return std::forward<Sndr>(sndr).apply(adapt::__when_all::make_state<Rcvr>());
        };

        static constexpr auto start = // NOLINT
            []<class State, class Rcvr, class... Ops>(State &state, Rcvr &rcvr,
                                                      Ops &...ops) noexcept -> void {
            // Note: 赋值 make_state::on_stop.值为 token + stop_callback的组合模板实例
            // 其中：on_stop_request 是 stop_callback。当stop_request发生时调用
            // Note: 标准规定，request_stop之后的 注册的callback，都立即调用
            state.register_stop_callback(queries::get_stop_token(queries::get_env(rcvr)));
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
            // TODO(mcs): 应该可以继续优化，裁剪掉 set_stopped
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

    template <typename... Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::when_all_t, snd::empty_data, Sndr...>, Env...>
    {
        using Sigs = decltype((snd::completion_signatures_of_t<Sndr, Env...>{} + ... +
                               cmplsigs::completion_signatures<>{})); // NOLINTNEXTLINE
        using All_E = decltype(Sigs::template filter_sigs<set_error_t>()); // NOLINTEND
        using All_S = decltype(Sigs::template filter_sigs<set_stopped_t>());

        static consteval auto get_all_v() // NOLINT
        {                                 // NOLINTNEXTLINE
            auto fun = []<typename Tag, typename... Ts>(Tag (*)(Ts...)) {
                if constexpr (std::is_same_v<Tag, set_value_t>)
                {
                    return std::tuple<Ts...>{};
                }
                else
                    return std::tuple<>{};
            };
            auto add_all = [&]<class... Sig>(cmplsigs::completion_signatures<Sig...>) {
                return std::tuple_cat(fun(static_cast<Sig *>(nullptr))...);
            };
            using tuple = decltype(std::tuple_cat(
                add_all(snd::completion_signatures_of_t<Sndr, Env...>{})...));

            return []<class... T>(std::tuple<T...>) {
                if constexpr (sizeof...(T) == 0)
                    return cmplsigs::completion_signatures<set_value_t()>{};
                else
                    return cmplsigs::completion_signatures<set_value_t(T...)>{};
            }(tuple{});
        };
        using type = decltype(get_all_v() + All_E{} + All_S{});
    };

    namespace diagnostics
    {
        template <typename... Sndrs, typename... Env>
        inline constexpr bool check_type_impl< // NOLINT
            snd::__detail::basic_sender<adapt::when_all_t, snd::empty_data, Sndrs...>,
            Env...> = []() consteval {
            using stop_source = stoptoken::finite_inplace_stop_source<sizeof...(Sndrs)>;
            using index = std::index_sequence_for<Sndrs...>;
            auto fn = []<typename Sndr, std::size_t I>() consteval {
                using CS =
                    decltype(snd::get_completion_signatures<
                             Sndr, decltype(snd::general::impls_for<adapt::when_all_t>::
                                                make_when_all_env<I>(
                                                    std::declval<stop_source>(),
                                                    std::declval<Env>()))...>());
                constexpr auto size = CS::template count<set_value_t>; // NOLINT
                if constexpr (size >= 2)
                    throw diagnostics::invalid_completion_signature<
                        IN_TAG(adapt::when_all_t), WITH_ENV(Env...),
                        NOTE_INFO(
                            cannot_accepts_a_sender_that_set_value_completion_signature_more_than_two)>();
            };
            [&fn]<std::size_t... Is>(std::index_sequence<Is...>) {
                (fn.template operator()<Sndrs, Is>(), ...);
            }(index{});
            return true;
        }();
    }; // namespace diagnostics

}; // namespace mcs::execution