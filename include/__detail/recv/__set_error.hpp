#pragma once

#include <type_traits>
#include <utility>

namespace mcs::execution::recv
{
    //[async.ops]
    struct set_error_t
    {
        template <typename R, typename Ts>
            requires(not(std::is_lvalue_reference_v<R> ||
                         (std::is_rvalue_reference_v<R> &&
                          std::is_const_v<std::remove_reference_t<R>>)))
        constexpr auto operator()(R &&rcvr, Ts &&ts) const noexcept
            requires(requires {
                { std::forward<R>(rcvr).set_error(std::forward<Ts>(ts)) } noexcept;
            })
        {
            return std::forward<R>(rcvr).set_error(std::forward<Ts>(ts));
        }
    };
    constexpr inline set_error_t set_error; // NOLINT

}; // namespace mcs::execution::recv
