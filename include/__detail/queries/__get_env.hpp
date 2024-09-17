#pragma once

#include <utility>

#include "../__core_concepts.hpp"

namespace mcs::execution::queries
{
    struct get_env_t
    {

        template <typename T>
            requires requires(T &&o) { // concepts sender defined need const T &
                { std::as_const(o).get_env() } noexcept;
            }
        constexpr auto operator()(const T &o) const noexcept -> queryable auto
        {
            return std::as_const(o).get_env();
        }
    };
    inline constexpr get_env_t get_env{}; // NOLINT

}; // namespace mcs::execution::queries