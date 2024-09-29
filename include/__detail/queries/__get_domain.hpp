#pragma once

#include "./__forwarding_query.hpp"

namespace mcs::execution::queries
{
    struct get_domain_t : forwarding_query_t
    {
        using __self_t = get_domain_t;

        template <typename T>
            requires requires(T &&env) {
                { std::as_const(env).query(std::declval<__self_t>()) } noexcept;
            }
        constexpr auto operator()(T &&env) const noexcept
        {
            return std::as_const(env).query(*this);
        }
    };
    inline constexpr get_domain_t get_domain{}; // NOLINT

}; // namespace mcs::execution::queries