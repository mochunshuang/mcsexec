#pragma once

#include <type_traits>
#include <utility>

namespace mcs::execution::recv
{
    //[async.ops]
    struct set_stopped_t
    {
        template <typename R>
            requires(not(std::is_lvalue_reference_v<R> ||
                         (std::is_rvalue_reference_v<R> &&
                          std::is_const_v<std::remove_reference_t<R>>)))
        constexpr auto operator()(R &&r) const noexcept
        {
            return std::forward<R>(r).set_stopped();
        }
    };
    constexpr inline set_stopped_t set_stopped; // NOLINT

}; // namespace mcs::execution::recv
