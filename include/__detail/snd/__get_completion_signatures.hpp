#pragma once

#include <exception>
#include <type_traits>
#include <utility>
#include "./general/__SET_VALUE_SIG.hpp"
#include "./general/__get_domain_late.hpp"

#include "../awaitables/__env_promise.hpp"
#include "../awaitables/__is_awaitable.hpp"
#include "../awaitables/__await_result_type.hpp"

#include "./__transform_sender.hpp"
#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::snd
{

    // Then the type of the expression get_completion_signatures(sndr, env) is a
    // specialization of the class template completion_signatures
    // ([exec.utils.cmplsigs]), the set of whose template arguments is Sigs
    /**
     * @brief   sender_in<Sndr, env_of_t<Rcvr>>:
                检查 Sndr 是否可以在 Rcvr 的环境中作为 sender 使用。

                completion_signatures_of_t<Sndr, env_of_t<Rcvr>>:
                获取 Sndr 在 Rcvr 环境中的完成签名。

                MATCHING-SIG(decayed-typeof<CSO>(decltype(args)...), Sig):
                检查 CSO 的类型（经过 decay 处理）和 args... 的类型是否与 Sig 匹配

        rcvr:
        一个右值，其类型 Rcvr 符合 receiver 模型。

        Sndr:
        一个 sender 类型，满足 sender_in<Sndr, env_of_t<Rcvr>> 为 true。

        Sigs...:
        completion_signatures_of_t<Sndr, env_of_t<Rcvr>> 的模板参数，表示 sender
     的完成签名。

        CSO:
        一个完成函数（completion function）。

    表达式 CSO(rcvr, args...) 可能被求值：
    如果 sender Sndr 或其操作状态导致表达式 CSO(rcvr, args...) 可能被求值（根据
    [basic.def.odr] 规则）。

    匹配签名：
    必须存在一个签名 Sig 在 Sigs... 中，使得
    MATCHING-SIG(decayed-typeof<CSO>(decltype(args)...), Sig) 为 true。
     *
     */
    struct get_completion_signatures_t
    {

        template <typename S, typename E>
        constexpr auto operator()(S &&sndr, E &&env) const noexcept
        {
            decltype(auto) tmp =
                transform_sender(decltype(general::get_domain_late(sndr, env)){}, //
                                 std::forward<S>(sndr), env);
            decltype(auto) new_sndr{std::forward<decltype(tmp)>(tmp)};

            using NewSndr = decltype((new_sndr));
            using Env = decltype((env));

            if constexpr (requires {
                              std::forward<decltype(new_sndr)>(new_sndr)
                                  .get_completion_signatures(std::forward<E>(env));
                          })
            {
                using CS = decltype(std::forward<decltype(new_sndr)>(new_sndr)
                                        .get_completion_signatures(std::forward<E>(env)));
                return (void(sndr), void(env), CS());
            }
            else if constexpr (requires {
                                   typename std::remove_cvref_t<
                                       NewSndr>::completion_signatures;
                               })
            {
                using CS = std::remove_cvref_t<NewSndr>::completion_signatures;
                return (void(sndr), void(env), CS());
            }
            else if constexpr (awaitables::is_awaitable<NewSndr,
                                                        awaitables::env_promise<Env>>)
            {
                using general::SET_VALUE_SIG;

                using CS = cmplsigs::completion_signatures<
                    SET_VALUE_SIG<awaitables::await_result_type<
                        NewSndr,
                        awaitables::env_promise<Env>>>, // see
                                                        // [exec.snd.concepts]
                    set_error_t(std::exception_ptr), set_stopped_t()>;
                return (void(sndr), void(env), CS());
            }
        }
    };
    constexpr inline get_completion_signatures_t get_completion_signatures{}; // NOLINT

}; // namespace mcs::execution::snd
