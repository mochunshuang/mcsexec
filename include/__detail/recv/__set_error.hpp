#pragma once

#include <type_traits>
#include <utility>

namespace mcs::execution::recv
{
    //[async.ops]
    struct set_error_t
    {
        template <typename R, typename Ts>
        constexpr auto operator()(R &&rcvr, Ts &&ts) const noexcept
            requires(std::is_rvalue_reference_v<decltype(rcvr)> &&
                     not std::is_const_v<std::remove_reference_t<decltype(rcvr)>> //
                     && requires {
                            {
                                std::forward<R>(rcvr).set_error(std::forward<Ts>(ts))
                            } noexcept;
                        })
        {
            return std::forward<R>(rcvr).set_error(std::forward<Ts>(ts));
        }
    };
    constexpr inline set_error_t set_error{}; // NOLINT

}; // namespace mcs::execution::recv
