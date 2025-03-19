#pragma once

#include "./mate_type/__state_type.hpp"
#include "../../__functional/__nothrow_callable.hpp"

namespace mcs::execution::snd::__detail
{
    template <class Sndr, class Rcvr>
    struct basic_state // exposition only
    {
        // NOTE: 异常计算 基于 p3388r1 回退 到 p2300r10 原始版本。sndr.conect(rcvr)
        // NOTE: 还是留着，这一大坨是判断 mate_type::state_type<Sndr, Rcvr> 是否无异常构造
        basic_state(Sndr &&sndr, Rcvr &&rcvr) noexcept(
            std::is_nothrow_move_constructible_v<Rcvr> &&
            functional::nothrow_callable<
                decltype(general::impls_for<tag_of_t<Sndr>>::get_state), Sndr, Rcvr &> &&
            static_cast<bool>(
                std::same_as<mate_type::state_type<Sndr, Rcvr>,
                             functional::call_result_t<
                                 decltype(general::impls_for<tag_of_t<Sndr>>::get_state),
                                 Sndr, Rcvr &>> ||
                std::is_nothrow_constructible_v<
                    mate_type::state_type<Sndr, Rcvr>,
                    functional::call_result_t<
                        decltype(general::impls_for<tag_of_t<Sndr>>::get_state), Sndr,
                        Rcvr &>>))
            : rcvr(std::move(rcvr)), state(general::impls_for<tag_of_t<Sndr>>::get_state(
                                         std::forward<Sndr>(sndr), this->rcvr))
        {
        }

        Rcvr rcvr;                               // exposition only // NOLINT
        mate_type::state_type<Sndr, Rcvr> state; // exposition only // NOLINT
    };
}; // namespace mcs::execution::snd::__detail