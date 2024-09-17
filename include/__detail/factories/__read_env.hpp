#pragma once

#include "../snd/__detail/__product_type.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__impls_for.hpp"

#include "../queries/__get_completion_scheduler.hpp"
#include "../queries/__get_env.hpp"

namespace mcs::execution
{

    namespace factories
    {
        // read from the receiver’s associated environment.
        struct read_env_t
        {
            snd::sender decltype(auto) constexpr operator()(auto &&q) const noexcept
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
        // TRY-SET-VALUE(rcvr, expr)
        using type =
            cmplsigs::completion_signatures<recv::set_value_t(decltype(std::declval<Q>()(
                                                ::std::as_const(::std::declval<Env>())))),
                                            recv::set_error_t(::std::exception_ptr)>;
    };

}; // namespace mcs::execution
