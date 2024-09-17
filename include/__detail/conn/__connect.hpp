#pragma once

#include "./__detail/__connect_awaitable_promise.hpp"
#include "./__detail/__connect_awaitable.hpp"

#include "../snd/__transform_sender.hpp"
#include "../snd/general/__SET_VALUE_SIG.hpp"
#include "../snd/general/__get_domain_late.hpp"

#include "../opstate/__operation_state.hpp"
#include "../queries/__get_env.hpp"

#include "../awaitables/__await_result_type.hpp"

namespace mcs::execution::conn
{
    /**
    Note:
        decltype(auto) new_sndr =
            transform_sender(decltype(get_domain_late(sndr,get_env(rcvr))){},
                            sndr, get_env(rcvr));
        using Sndr = decltype((sndr));
        using Rcvr = decltype((rcvr));
        using DS = std::decay_t<decltype((new_sndr))>;
        using DR = std::decay_t<Rcvr>;
    */
    struct connect_t
    {
        template <typename Sndr, typename Rcvr>
        constexpr auto operator()(Sndr &&sndr, Rcvr rcvr) const noexcept
            requires(snd::sender<decltype((sndr))> && receiver<decltype((rcvr))>)
        {

            decltype(auto) new_sndr = snd::transform_sender(
                decltype(snd::general::get_domain_late(sndr, queries::get_env(rcvr))){},
                std::forward<Sndr>(sndr), queries::get_env(rcvr));

            if constexpr (requires {
                              {
                                  new_sndr.connect(std::move(rcvr))
                              } -> opstate::operation_state;
                          })
            {
                return new_sndr.connect(std::move(rcvr)); // copy
            }
            else
            {
                using __detail::connect_awaitable_promise;
                using __detail::connect_awaitable;

                using snd::general::SET_VALUE_SIG;
                using cmplsigs::completion_signatures;
                using awaitables::await_result_type;

                using RCVR = decltype((rcvr));
                using DS = std::decay_t<decltype((new_sndr))>;
                using DR = std::decay_t<RCVR>;
                using V = await_result_type<DS, connect_awaitable_promise<DS, DR>>;

                using Sigs =
                    completion_signatures<SET_VALUE_SIG<V>, // see [exec.snd.concepts]
                                          set_error_t(std::exception_ptr),
                                          set_stopped_t()>;

                using operation_state_task =
                    connect_awaitable_promise<DS, DR>::operation_state_task;

                return connect_awaitable<operation_state_task, DS, DR, Sigs, V>(
                    std::move(new_sndr), std::move(rcvr));
            }
        };
    };

    constexpr inline connect_t connect{}; // NOLINT
}; // namespace mcs::execution::conn
