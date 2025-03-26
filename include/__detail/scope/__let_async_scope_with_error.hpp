#pragma once

#include "../__functional/__decayed_typeof.hpp"
#include "../snd/__sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/general/__get_domain_early.hpp"

#include "../snd/general/__SCHED_ENV.hpp"
#include "../snd/general/__MAKE_ENV.hpp"
#include "../snd/general/__JOIN_ENV.hpp"
#include "../snd/general/__emplace_from.hpp"

#include "./__counting_scope.hpp"
#include <algorithm>
#include <exception>
#include <type_traits>
#include <variant>

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../conn/__connect_result_t.hpp"

#include "./__nest.hpp"

#include "../adapt/__when_all_with_variant.hpp"
#include "../adapt/__then.hpp"

namespace mcs::execution
{
    namespace scope
    {
        constexpr auto let_async_scope_env(snd::sender auto const &sndr) noexcept
        {
            if constexpr (requires {
                              snd::general::SCHED_ENV(
                                  queries::get_completion_scheduler<
                                      functional::decayed_typeof<set_value>>(
                                      queries::get_env(sndr)));
                          })
            {
                return snd::general::SCHED_ENV(
                    queries::get_completion_scheduler<
                        functional::decayed_typeof<set_value>>(queries::get_env(sndr)));
            }
            else if constexpr (requires {
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
                return empty_env{};
            }
        };

        template <typename... Errors>
        struct let_async_scope_with_error_t
        {
            template <snd::sender Sndr, movable_value Fun>
            auto operator()(Sndr &&sndr, Fun &&f) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(dom,
                                             snd::make_sender(*this, std::forward<Fun>(f),
                                                              std::forward<Sndr>(sndr)));
            }

            template <snd::sender Sndr, typename Env>
                requires snd::sender_for<Sndr, let_async_scope_with_error_t<Errors...>>
            auto transform_env(Sndr &&sndr, Env &&env) noexcept // NOLINT
            {
                return snd::general::JOIN_ENV(let_async_scope_env(sndr),
                                              snd::general::FWD_ENV(env));
            }
        };

        template <typename... E> // NOLINTNEXTLINE
        inline constexpr let_async_scope_with_error_t<E...> let_async_scope_with_error{};
    }; // namespace scope

    namespace scope::__detail
    {
        using scope_type = scope::counting_scope;
        // NOTE: scope-token-type shall be a unique type
        using scope_token_type = decltype(std::declval<scope_type>().get_token());

        template <class Rcvr, class Env>
        struct receiver2
        {
            using receiver_concept = receiver_t;

            explicit receiver2(Rcvr rcvr, Env env) noexcept
                : rcvr(std::move(rcvr)), env(std::move(env))
            {
            }

            auto get_env() const noexcept // NOLINT
            {
                return snd::general::JOIN_ENV(
                    env, snd::general::FWD_ENV(queries::get_env(rcvr)));
            }

            template <class... Args>
            void set_value(std::variant<std::tuple<Args...>> &&t) && noexcept // NOLINT
            {
                std::apply(
                    [this](auto &&...args) {
                        recv::set_value(std::move(rcvr),
                                        std::forward<decltype(args)>(args)...);
                    },
                    std::get<std::tuple<Args...>>(std::move(t)));
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

            Rcvr rcvr; // NOLINT
            Env env;   // NOLINT
        };

        template <typename scope_token_type, class Sigs>
        static consteval auto get_variant()
        {

            return []<typename... Sig>(cmplsigs::completion_signatures<Sig...>) {
                auto as_tuple = []<typename Tag, typename... Args>(
                                    Tag (*)(Args...)) consteval {
                    if constexpr (requires {
                                      typename decayed_tuple<scope_token_type, Args...>;
                                  })
                    {
                        using T = decayed_tuple<scope_token_type, Args...>;
                        return static_cast<T *>(nullptr);
                    }
                    else
                        throw;
                };
                using T = typename tfxcmplsigs::unique_variadic_template<std::variant<
                    std::monostate, std::remove_pointer_t<decltype(as_tuple(
                                        static_cast<Sig *>(nullptr)))>...>>::type;
                return T{};
            }(Sigs{});
        };

        template <typename Fn, typename scope_token_type, typename Sig>
        struct as_sndr2;
        template <typename Fn, typename scope_token_type, typename Tag, typename... Args>
        struct as_sndr2<Fn, scope_token_type, Tag(Args...)>
        {
            using type = functional::call_result_t<Fn, std::decay_t<scope_token_type> &,
                                                   std::decay_t<Args> &...>;
            static constexpr bool is_nothrow = // NOLINT
                std::is_nothrow_invocable_v<Fn, std::decay_t<scope_token_type> &,
                                            std::decay_t<Args> &...>;
        };

        struct then_functor
        {
            template <typename T, typename U>
            auto operator()(T &&result, U && /*unused*/) const noexcept -> decltype(auto)
            {
                return std::forward<T>(result);
            }
        };

        template <typename Fn, typename scope_token_type, typename Sigs>
        static consteval auto is_fn_nothrow()
        {
            auto compute = []<typename Tag, typename... Args>(
                               Tag (*)(Args...)) consteval {
                if constexpr (std::is_same_v<Tag, set_value_t>)
                {
                    if constexpr (requires {
                                      typename as_sndr2<Fn, scope_token_type,
                                                        Tag(Args...)>::type;
                                  })
                    {
                        using Ret = as_sndr2<Fn, scope_token_type, Tag(Args...)>::type;
                        static_assert(snd::sender<Ret>, "T not a sndr");
                        return as_sndr2<Fn, scope_token_type, Tag(Args...)>::is_nothrow;
                    }
                    else
                        throw;
                }
                else
                    return true;
            };
            auto compute_all =
                [&]<typename... Sig>(cmplsigs::completion_signatures<Sig...>) consteval {
                    return (compute(static_cast<Sig *>(nullptr)) && ...);
                };
            return compute_all(Sigs{});
        }

        template <typename join_sender, typename Fn, typename scope_token_type,
                  typename Sigs, typename Rcvr, typename Env>
        static consteval auto compute_ops2_variant()
        {
            auto compute_sndr = []<typename Tag, typename... Args>(
                                    Tag (*)(Args...)) consteval {
                if constexpr (requires {
                                  typename as_sndr2<Fn, scope_token_type,
                                                    Tag(Args...)>::type;
                              })
                {
                    using Ret = as_sndr2<Fn, scope_token_type, Tag(Args...)>::type;
                    static_assert(snd::sender<Ret>, "T not a sndr");
                    using sndr2 = decltype(nest(std::declval<Ret>(),
                                                std::declval<scope_token_type>()));
                    static_assert(snd::sender<sndr2>, "sndr2 not a sndr");
                    using result_sender =
                        decltype(adapt::when_all_with_variant(
                                     std::declval<sndr2>(), std::declval<join_sender>()) |
                                 adapt::then(then_functor()));
                    // return result_sender{};
                    return static_cast<result_sender *>(nullptr);
                }
                else
                    throw;
            };

            auto compute_all = [&]<typename... Sig>(
                                   cmplsigs::completion_signatures<Sig...>) consteval {
                static_assert((snd::sender<std::remove_pointer_t<decltype(compute_sndr(
                                   static_cast<Sig *>(nullptr)))>> &&
                               ...),
                              "fn return not a Sndr");
                using VariantType =
                    typename tfxcmplsigs::unique_variadic_template<std::variant<
                        std::monostate, conn::connect_result_t<
                                            std::remove_pointer_t<decltype(compute_sndr(
                                                static_cast<Sig *>(nullptr)))>,
                                            receiver2<Rcvr, Env>>...>>::type;
                return VariantType{};
            };
            return compute_all(Sigs{});
        }

        template <typename error_variant_type, typename child_type, typename Rcvr>
        constexpr static auto get_let_async_scope_with_error_state = // NOLINT
            []<typename Fn, class Env>(Fn fn, Env env) noexcept {
                using Sigs = snd::completion_signatures_of_t<
                    child_type, decltype(snd::general::FWD_ENV(
                                    queries::get_env(std::declval<Rcvr &>())))>;
                using LetSigs = decltype(Sigs::template filter_sigs<set_value_t>());
                using args_variant_type =
                    decltype(scope::__detail::get_variant<scope_token_type, LetSigs>());

                using scope_type = scope::__detail::scope_type;
                using scope_token_type = scope::__detail::scope_token_type;

                using join_sender = decltype(std::declval<scope_type>().join());
                using ops2_variant_type = std::remove_pointer_t<
                    decltype(scope::__detail::compute_ops2_variant<
                             join_sender, Fn, scope_token_type, LetSigs, Rcvr, Env>())>;

                struct state_type
                {
                    using nothrow [[maybe_unused]] = std::integral_constant<
                        bool, is_fn_nothrow<Fn, scope_token_type, LetSigs>()>;
                    Fn fn;
                    Env env;
                    scope_type scope;
                    args_variant_type args;
                    ops2_variant_type ops2;
                    std::optional<error_variant_type> error;
                };
                return state_type{std::move(fn), std::move(env), {}, {}, {}, {}};
            };

    }; // namespace scope::__detail

    namespace scope
    {
        template <class State, class Rcvr, class... Args>
        inline void let_async_scope_bind(State &state, Rcvr &rcvr,
                                         Args &&...args) noexcept // exposition only
        {
            // NOLINTNEXTLINE
            constexpr bool is_nothrow = std::decay_t<decltype(state)>::nothrow::value;
            using scope_token_type = decltype(state.scope.get_token());
            auto create_scope_token = [&]() noexcept { // NOLINT
                return state.scope.get_token();
            };

            auto &args_variant =
                state.args.template emplace<decayed_tuple<scope_token_type, Args...>>(
                    create_scope_token(), std::forward<Args>(args)...);

            if constexpr (is_nothrow)
            {
                auto sndr2 = [&] noexcept {
                    return nest(std::apply(std::move(state.fn), args_variant),
                                state.scope.get_token());
                };
                auto result_sender = [&] noexcept {
                    return adapt::when_all_with_variant(sndr2(), state.scope.join()) |
                           adapt::then(scope::__detail::then_functor());
                };
                auto mkop2 = [&] noexcept {
                    return conn::connect(
                        result_sender(),
                        __detail::receiver2{std::move(rcvr), std::move(state.env)});
                };
                // NOTE: ops2 的op类型要和mkop2返回的类型匹配
                auto &op2 = state.ops2.template emplace<decltype(mkop2())>(
                    snd::general::emplace_from{mkop2});
                opstate::start(op2);
            }
            else
            {
                try
                {
                    auto sndr2 = [&] {
                        return nest(std::apply(std::move(state.fn), args_variant),
                                    state.scope.get_token());
                    };
                    auto result_sender = [&] {
                        return adapt::when_all_with_variant(sndr2(), state.scope.join()) |
                               adapt::then(scope::__detail::then_functor());
                    };
                    auto mkop2 = [&] {
                        return conn::connect(
                            result_sender(),
                            __detail::receiver2{std::move(rcvr), std::move(state.env)});
                    };
                    // NOTE: ops2 的op类型要和mkop2返回的类型匹配
                    auto &op2 = state.ops2.template emplace<decltype(mkop2())>(
                        snd::general::emplace_from{mkop2});
                    opstate::start(op2);
                }
                catch (...)
                {
                    // TODO(mcs): 异常通道暂时无法处理.直接简单处理
                    // state.scope.request_stop();
                    // auto result_sender = adapt::when_all(
                    //     factories::just_error(std::current_exception()),
                    //     state.scope.join());
                    // auto rcvr2 = __detail::receiver2{std::move(rcvr),
                    // std::move(state.env)}; auto mkop2 = [&] noexcept { // NOLINT
                    //     return conn::connect(std::move(result_sender),
                    //     std::move(rcvr2));
                    // };
                    // auto &op2 = state.ops2.template emplace<decltype(mkop2())>(
                    //     snd::general::emplace_from{mkop2});
                    // opstate::start(op2);
                    state.scope.request_stop();
                    recv::set_error(std::move(rcvr), std::current_exception());
                }
            }
        }
    }; // namespace scope

    template <typename... Errors>
    struct snd::general::impls_for<scope::let_async_scope_with_error_t<Errors...>>
        : snd::__detail::default_impls
    {
        /**
         * @brief
         * 1. scope-type is a type that behaves like a counting_scope, except that it is
         * never closed.
         *
         */
        static constexpr auto get_state = // NOLINT
            []<class OutSndr, class Rcvr>(OutSndr &&out_sndr, Rcvr & /*rcvr*/) noexcept {
                using error_variant_type = typename tfxcmplsigs::unique_variadic_template<
                    std::variant<Errors...>>::type;
                auto &&[tag, fun, sndr] = std::forward<OutSndr>(out_sndr);
                using child_type = std::decay_t<decltype(sndr)>;
                return scope::__detail::get_let_async_scope_with_error_state<
                    error_variant_type, child_type, Rcvr>(
                    std::forward_like<OutSndr>(fun), scope::let_async_scope_env(sndr));
            };

        static constexpr auto complete = // NOLINT
            []<class Tag, class... Args>(auto, auto &state, auto &rcvr, Tag,
                                         Args &&...args) noexcept -> void {
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                // NOTE: It's already try-catch in let_async_scope_bind
                scope::let_async_scope_bind(state, rcvr, std::forward<Args>(args)...);
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    template <typename Sndr, typename Fun, typename... Env, typename... Errors>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<scope::let_async_scope_with_error_t<Errors...>, Fun,
                                    Sndr>,
        Env...>
    {
        using Sigs = snd::completion_signatures_of_t<Sndr, Env...>;
        static consteval auto getSigs()
        {
            auto compute = []<typename Tag, typename... Args>(
                               Tag (*)(Args...)) consteval {
                if constexpr (std::is_same_v<Tag, set_value_t>)
                {
                    if constexpr (requires {
                                      typename scope::__detail::as_sndr2<
                                          Fun, scope::__detail::scope_token_type,
                                          Tag(Args...)>::type;
                                  })
                    {
                        using Ret = scope::__detail::as_sndr2<
                            Fun, scope::__detail::scope_token_type, Tag(Args...)>::type;
                        static_assert(snd::sender<Ret>, "T not a sndr");
                        return snd::completion_signatures_of_t<Ret, Env...>{};
                    }
                    else
                        throw;
                }
                else
                {
                    return cmplsigs::completion_signatures<Tag(Args...)>{};
                }
            };
            auto compute_all =
                [&]<typename... Sig>(cmplsigs::completion_signatures<Sig...>) consteval {
                    return (compute(static_cast<Sig *>(nullptr)) + ... +
                            cmplsigs::completion_signatures<>{});
                };
            return compute_all(Sigs{});
        };
        template <typename... E>
        static consteval auto addSigs()
        {
            return cmplsigs::completion_signatures<set_error_t(E)...>{};
        }
        using type = decltype(getSigs() + addSigs<Errors...>());
    };

}; // namespace mcs::execution