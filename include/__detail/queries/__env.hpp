#pragma once

#include "../__core_concepts.hpp"
#include <utility>

namespace mcs::execution::queries
{
    template <class Env, class QueryTag>
    concept has_query = // exposition only
        requires(const Env &env) { env.query(QueryTag()); };

    template <queryable... Envs>
    struct env : Envs...
    {
        constexpr explicit env(Envs... envs) noexcept : Envs(std::forward<Envs>(envs))...
        {
        }

        template <class QueryTag>
        constexpr decltype(auto) query(QueryTag q) const noexcept
        {
            return query_impl<QueryTag>(q, static_cast<const Envs &>(*this)...);
        }

      private:
        template <class QueryTag, typename FirstEnv, typename... RestEnvs>
        constexpr decltype(auto) query_impl(QueryTag q, // NOLINT
                                            const FirstEnv &first,
                                            const RestEnvs &...rest) const noexcept
        {
            if constexpr (has_query<FirstEnv, QueryTag>)
            {
                return first.query(q);
            }
            else if constexpr (sizeof...(RestEnvs) > 0)
            {
                return query_impl(q, rest...);
            }
            else
            {
                static_assert(sizeof...(RestEnvs) > 0, "No matching query found!");
            }
        }
    };

    template <class... Envs>
    env(Envs...) -> env<std::unwrap_reference_t<Envs>...>;

}; // namespace mcs::execution::queries
