#pragma once

#include "../__default_domain.hpp"

#include "../../queries/__get_env.hpp"
#include "../../queries/__get_domain.hpp"

namespace mcs::execution::snd::general
{

    // TODO(mcs): 标准并没有明确定义,猜的
    template <typename Sch>
    constexpr auto query_or_default(queries::get_domain_t const & /*unused*/,
                                    const Sch &sch, const default_domain & /*unused*/)
    {
        if constexpr (requires { queries::get_domain(queries::get_env(sch)); })
        {
            return queries::get_domain(queries::get_env(sch));
        }
        else
        {
            return default_domain();
        }
    };

    template <typename expr>
    concept not_void = not std::is_same_v<expr, void>;

}; // namespace mcs::execution::snd::general