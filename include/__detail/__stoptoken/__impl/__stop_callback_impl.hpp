#pragma once

#include "./../__stop_callback.hpp"
#include "./../__stoppable_callback_for.hpp"
#include "./../__stop_callback_for_t.hpp"
#include <utility>

/**
 * @brief 没有用到，不用写实现。 stop_token
 * c++标准库早就有了，不太可能重新写。仅仅是建模展示
 *
 */
namespace mcs::execution::stoptoken
{
    /**
     * @brief 仅仅是搭建框架范围。 inplace_stop_XXX 才是具体的,
     * CallbackFn 和 Initializer都是模板。他们的关系正确性由模板概念约束
     *
     */
    // Note: 模板构造函数是为了要求: std::constructible_from<stop_callback_for_t<Token,
    // Note: CallbackFn>,const Token &,Initializer >
    //
    // Note: stop_callback_for_t<Token, CallbackFn> == typename Token::template
    // Note: callback_type<CallbackFn>
    // Note: 其实就是要求 Token 和 callback_type == CallbackFn 的关系
    // Note: CallbackFn 在 [stop_callback]、[stop_token] 都是模板实参
    // Note: stoppable_callback_for 的明显取自 [stop_callback] 和 [stop_token]建模
    template <__detail::invocable_destructible CallbackFn>
    template <class Initializer>
    inline stop_callback<CallbackFn>::stop_callback(
        const stop_token &st,
        Initializer
            &&init) noexcept(std::is_nothrow_constructible_v<CallbackFn, Initializer>)
    {
        // 1.
        // Note: 保证 CallbackFn空参调用，上面已经限制。因此以下校验是不需要的。
        // Note: 因为：[stoppable_callback_for] 约束的余下部分，就是当前构造函数正在做的
        // Note: 不能放在头文件中。  requires 也是签名的一部分。定义和实现签名要一致
        // static_assert(std::same_as<decltype(init), Initializer> &&
        //               stoppable_callback_for<CallbackFn, stop_token, Initializer>);

        // 确保 SCB 可以被以下类型构造
        // using SCB = typename T::template callback_type<CallbackFn>
        // Note: SCB 就是当前类型，stop_token 就是特例罢了。约束 CallbackFn,Initializer
        // using SCB = stop_callback_for_t<stop_token, CallbackFn>;
        // static_assert(std::constructible_from<SCB, stop_token, Initializer>);
        // static_assert(std::constructible_from<SCB, stop_token &, Initializer>);
        // static_assert(std::constructible_from<SCB, const stop_token, Initializer>);

        // 2. .......
        // TODO(mcs):
        // 以上都是要求，不是介绍该函数时的要求时要求的。是上面概念
        // 【stoppable_callback_for】 要求的

        // 1. stoppable_callback_for 已经要求下面必须满足
        // Constraints: CallbackFn and Initializer satisfy
        // constructible_from<CallbackFn,Initializer>.

        // 2. Initializes callback-fn with std::forward<Initializer>(init) and
        // executes a stoppable callback registration ([stoptoken.concepts]).
        // If a callback is registered with st's shared stop state, then *this acquires
        // shared ownership of that stop state.
        // 2.1 Initializes callback-fn; 有条件的
        // Note: [stoppable callback registration]
        if (st.stop_possible())
        {
            this->callback_fn(std::forward<Initializer>(init));
            // 2.2 executes a stoppable callback registration
            // std::forward<CallbackFn>(callback_fn)(); // Note: 表示调用request_stop()
            // Note: CallbackFn 规范是 snd::general::on_stop_request
            // Note: 即：stop_src.request_stop();
            // TODO(mcs): 描述说的很模糊，看来得实现 inplace_stop_XXX 才知道
            // 要求：If a callback is registered with st's shared stop state, then *this
            // acquires shared ownership of that stop state.
            // TODO(mcs): 如何保证 *this 获得所有权呢
        }
        // Note: If t.stop_possible() is false, there is no requirement that the
        // Note: initialization of scb causes the initialization of callback_fn
    }

    template <__detail::invocable_destructible CallbackFn>
    template <class Initializer>
    inline stop_callback<CallbackFn>::stop_callback(
        stop_token &&st,
        Initializer
            &&init) noexcept(std::is_nothrow_constructible_v<CallbackFn, Initializer>)
    {
        if (st.stop_possible())
        {
            this->callback_fn(std::forward<Initializer>(init));
        }
    }
    template <__detail::invocable_destructible CallbackFn>
    inline stop_callback<CallbackFn>::~stop_callback()
    {
        // Executes a stoppable callback deregistration ([stoptoken.concepts]) and
        // releases ownership of the stop state, if any.
        // TODO(mcs):
    }

}; // namespace mcs::execution::stoptoken