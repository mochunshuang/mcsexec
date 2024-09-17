#pragma once

#include <exception>
#include <type_traits>
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
    struct get_completion_signatures_t
    {

        template <typename S, typename E>
        constexpr auto operator()(S &&sndr, E &&env) const noexcept
        {
            decltype(auto) new_sndr =
                transform_sender(decltype(general::get_domain_late(sndr, env)){}, //
                                 std::forward<S>(sndr), env);

            using NewSndr = decltype((new_sndr));
            using Env = decltype((env));

            if constexpr (requires {
                              new_sndr.get_completion_signatures(std::forward<E>(env));
                          })
            {
                using CS =
                    decltype(new_sndr.get_completion_signatures(std::forward<E>(env)));
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
