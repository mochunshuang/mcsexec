#pragma once

#include <utility>

namespace mcs::execution::queries
{
    // [exec.queries], queries
    struct forwarding_query_t
    {
        template <typename Q>
            requires requires(Q &q) {
                { q.query(std::declval<forwarding_query_t>()) } noexcept;
            }
        constexpr auto operator()(Q &&q) const noexcept
        {
            return std::forward<Q>(q).query(*this);
        }

        template <typename Q>
        consteval auto operator()(Q && /*q*/) const noexcept -> bool
        {
            return std::derived_from<Q, forwarding_query_t>;
        }
    };

    inline constexpr forwarding_query_t forwarding_query{}; // NOLINT

}; // namespace mcs::execution::queries