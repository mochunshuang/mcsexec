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
            basic_state<Sndr, Rcvr> *op, Sndr &&sndr,
            std::index_sequence<Is...>) noexcept(true) -> decltype(auto) {
        // Note: sndr must be lvalue <=> auto&; std::forward_like<Sndr> => obj <=> obj.mb
        // Note: c++26 形参包： p1061r10 搞定，就不用那么麻烦了
        return sndr.apply(
            // Note: 至少 g++ 目前是无法计算noexcept的
            //  __connect_all.hpp:27:13: internal compiler error: Segmentation fault
            [&]<typename... Child>(auto &, auto &, Child &...child) noexcept {
                // product_type 0构造自定义，直接初始化是不会有异常的
                static_assert(noexcept(product_type{conn::connect(
                    std::forward_like<Sndr>(child),
                    basic_receiver<Sndr, Rcvr, std::integral_constant<std::size_t, Is>>{
                        op})...}));
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