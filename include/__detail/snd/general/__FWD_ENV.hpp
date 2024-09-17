#pragma once
#include "../../__core_concepts.hpp"
#include "../../queries/__forwarding_query.hpp"

namespace mcs::execution::snd::general
{
    template <typename Env>
    class FWD_ENV // NOLINT
    {
        Env env; // NOLINT

      public:
        explicit FWD_ENV(Env &&env) : env(std::forward<Env>(env)) {}

        template <queryable Q, typename... As>
            requires(queries::forwarding_query(std::remove_cvref_t<Q>())) &&
                    requires(const Q &q, As &&...args) {
                        env.query(q, std::forward<As>(args)...);
                    }
        constexpr auto query(const Q &q, As &&...as) const noexcept

        {
            return env.query(q, std::forward<As>(as)...);
        }

        template <queryable Q, typename... As>
            requires(not queries::forwarding_query(std::remove_cvref_t<Q>()))
        constexpr auto query(const Q &q, As &&...as) const noexcept =
            delete; // query is not forwardable
    };

    template <typename Env>
    FWD_ENV(Env &&) -> FWD_ENV<Env>;

}; // namespace mcs::execution::snd::general