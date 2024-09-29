#pragma once

#include <type_traits>
#include <utility>

namespace mcs::execution::recv
{
    //[async.ops]
    struct set_value_t
    {
        template <typename R, typename... Ts>
        constexpr auto operator()(R &&rcvr, Ts &&...vs) const noexcept
            requires(std::is_rvalue_reference_v<decltype(rcvr)> &&
                     not std::is_const_v<std::remove_reference_t<decltype(rcvr)>> //
                     && requires {
                            {
                                std::forward<R>(rcvr).set_value(std::forward<Ts>(vs)...)
                            } noexcept;
                        })
        {
            return std::forward<R>(rcvr).set_value(std::forward<Ts>(vs)...);
        }
    };
    constexpr inline set_value_t set_value{}; // NOLINT

}; // namespace mcs::execution::recv