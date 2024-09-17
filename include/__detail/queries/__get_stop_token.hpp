#pragma once

#include "../../stop_token.hpp"
#include "./__forwarding_query.hpp"

namespace mcs::execution::queries
{
    struct get_stop_token_t : forwarding_query_t
    {
        using __self_t = get_stop_token_t;
        template <typename T>
            requires requires(T &&env) {
                { std::as_const(env).query(std::declval<__self_t>()) } noexcept;
            }
        constexpr auto operator()(T &&env) const noexcept -> stoppable_token auto
        {
            return std::as_const(env).query(*this);
        }

        constexpr auto operator()(auto && /*unused*/) const -> never_stop_token
        {
            return {};
        }
    };
    inline constexpr get_stop_token_t get_stop_token{}; // NOLINT

}; // namespace mcs::execution::queries