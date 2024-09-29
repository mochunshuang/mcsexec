#pragma once

#include <type_traits>
#include <utility>

namespace mcs::execution::recv
{
    //[async.ops]
    struct set_stopped_t
    {
        template <typename R>
        constexpr auto operator()(R &&rcvr) const noexcept
            requires(std::is_rvalue_reference_v<decltype(rcvr)> &&
                     not std::is_const_v<std::remove_reference_t<decltype(rcvr)>>)
        {
            return std::forward<R>(rcvr).set_stopped();
        }
    };
    constexpr inline set_stopped_t set_stopped{}; // NOLINT

}; // namespace mcs::execution::recv
