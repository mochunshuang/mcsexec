#pragma once

#include "./__let_async_scope_with_error.hpp"
#include <utility>

namespace mcs::execution
{
    namespace scope
    {
        struct let_async_scope_t
        {
            template <snd::sender Sndr, movable_value Fun>
            auto operator()(Sndr &&sndr, Fun &&f) const noexcept
            {
                return let_async_scope_with_error<std::exception_ptr>(
                    std::forward<Sndr>(sndr), std::forward<Fun>(f));
            }
            template <movable_value Fun>
            auto operator()(Fun &&fun) const
                -> pipeable::sender_adaptor<let_async_scope_t, Fun>
            {
                return {*this, std::forward<Fun>(fun)};
            }
        };
        constexpr inline let_async_scope_t let_async_scope{}; // NOLINT
    }; // namespace scope

}; // namespace mcs::execution