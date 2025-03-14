#pragma once

#include <utility>

#include "../__core_concepts.hpp"
#include "../__core_types.hpp"

namespace mcs::execution::queries
{
    struct get_env_t
    {
        template <typename T>
        constexpr auto operator()(const T &o) const noexcept -> queryable auto
        {
            if constexpr (requires(T &&o) {
                              { std::as_const(o).get_env() } noexcept;
                          })
                return o.get_env();
            else
                return empty_env{};
        }
    };
    inline constexpr get_env_t get_env{}; // NOLINT

}; // namespace mcs::execution::queries