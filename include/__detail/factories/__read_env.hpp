#pragma once

#include "../snd/__make_sender.hpp"

#include "../snd/general/__impls_for.hpp"

#include "../queries/__get_env.hpp"
#include <type_traits>

namespace mcs::execution
{

    namespace factories
    {
        /**
         * read_env is a sender factory for a sender whose asynchronous operation
         * completes synchronously in its start operation with a value completion result
         * equal to a value read from the receiver’s associated environment.
         */
        struct read_env_t
        {
            snd::sender auto constexpr operator()(auto &&q) const noexcept
            {
                return snd::make_sender(*this, q);
            }
        };
        inline constexpr read_env_t read_env{}; // NOLINT
    }; // namespace factories

    template <>
    struct snd::general::impls_for<factories::read_env_t> : snd::__detail::default_impls
    {
        static constexpr auto start = // NOLINT
            [](auto query, auto &rcvr) noexcept -> void {
            //  TRY-SET-VALUE(rcvr, query(get_env(rcvr)));
            // TRY-SET-VALUE(rcvr, expr) is TRY-EVAL(rcvr, SET-VALUE(rcvr, expr))
            try
            {
                // SET-VALUE(rcvr, expr)
                if constexpr (std::is_void_v<decltype(query(queries::get_env(rcvr)))>)
                {
                    query(get_env(rcvr));
                    recv::set_value(std::move(rcvr));
                }
                else
                {
                    recv::set_value(std::move(rcvr), query(queries::get_env(rcvr)));
                }
            }
            catch (...)
            {
                recv::set_error(std::move(rcvr), std::current_exception());
            }
        };
    };

    template <typename Q, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<factories::read_env_t, Q>, Env>
    {
        // Note: sender_in provide Env, and call get_completion_signatures then call this
        using Add_Sig = recv::set_error_t(std::exception_ptr);
        using V_sig = std::conditional_t<std::is_void_v<decltype(std::declval<Q>()(
                                             ::std::as_const(::std::declval<Env &>())))>,
                                         recv::set_value_t(),
                                         recv::set_value_t(decltype(std::declval<Q>()(
                                             ::std::as_const(::std::declval<Env &>()))))>;
        using type = cmplsigs::completion_signatures<V_sig, Add_Sig>;
    };

}; // namespace mcs::execution
