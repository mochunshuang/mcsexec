#pragma once

#include "../__default_domain.hpp"
#include "./__completion_domain.hpp"

namespace mcs::execution::snd::general
{
    ////////////////////////////////////////
    // get_domain_early
    template <class Sndr>
    constexpr auto get_domain_early(const Sndr &sndr) noexcept
    {
        if constexpr (requires { queries::get_domain(queries::get_env(sndr)); })
        {
            return std::decay_t<decltype(queries::get_domain(queries::get_env(sndr)))>();
        }
        else if constexpr (requires { completion_domain(sndr); })
        {
            return std::decay_t<decltype(completion_domain(sndr))>();
        }
        else
        {
            return std::decay_t<default_domain()>();
        }
    }

}; // namespace mcs::execution::snd::general
