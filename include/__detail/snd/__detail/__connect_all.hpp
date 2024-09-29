#pragma once

#include "./__basic_receiver.hpp"
#include "./__basic_state.hpp"
#include "./__product_type.hpp"
#include "./mate_type/__indices_for.hpp"

#include "../../conn/__connect.hpp"

namespace mcs::execution::snd::__detail
{
    constexpr auto connect_all = // NOLINT
        []<class Sndr, class Rcvr, std::size_t... Is>(
            basic_state<Sndr, Rcvr> *op, Sndr &&sndr, // Note: noexcept can`t calculation
            std::index_sequence<Is...>) noexcept(true) -> decltype(auto) {
        // Note: sndr must be lvalue <=> auto&; std::forward_like<Sndr> => obj <=> obj.mb
        return sndr.apply(
            [&]<typename... Child>(auto &, auto &,
                                   Child &...child) noexcept -> decltype(auto) {
                return product_type{conn::connect(
                    std::forward_like<Sndr>(child),
                    basic_receiver<Sndr, Rcvr, std::integral_constant<std::size_t, Is>>{
                        op})...};
            });
    };

    template <class Sndr, class Rcvr>
    using connect_all_result = functional::call_result_t< // exposition only
        decltype(connect_all), basic_state<Sndr, Rcvr> *, Sndr,
        mate_type::indices_for<Sndr>>;

}; // namespace mcs::execution::snd::__detail