#pragma once
#include "../../__core_concepts.hpp"
#include <utility>

namespace mcs::execution::snd::general
{
    // other
    ///////////////////////////////////////////////////////
    // TODO(mcs): 是根据描述自己实现
    template <typename Env1, typename Env2>
    class JOIN_ENV // NOLINT
    {
        Env1 env1; // NOLINT
        Env2 env2; // NOLINT

      public:
        template <typename E1, typename E2>
        JOIN_ENV(E1 &&e1, E2 &&e2)
            : env1(std::forward<E1>(e1)), env2(std::forward<E2>(e2))
        {
        }

        JOIN_ENV() = delete;
        ~JOIN_ENV() noexcept = default;
        JOIN_ENV(const JOIN_ENV &) = default;
        JOIN_ENV(JOIN_ENV &&) noexcept = default;
        JOIN_ENV &operator=(const JOIN_ENV &) = default;
        JOIN_ENV &operator=(JOIN_ENV &&) noexcept = default;

        template <queryable Q, typename... As>
        constexpr auto query(const Q &q, As &&...as) const noexcept
            requires(
                requires() { env1.query(q, std::forward<As>(as)...); } ||
                requires() { env2.query(q, std::forward<As>(as)...); })
        {
            if constexpr (requires { env1.query(q, std::forward<As>(as)...); })
            {
                return env1.query(q, std::forward<As>(as)...);
            }
            else if constexpr (requires { env2.query(q, std::forward<As>(as)...); })
            {
                return env2.query(q, std::forward<As>(as)...);
            }
        }
    };

    template <typename Env1, typename Env2>
    JOIN_ENV(Env1 &&,
             Env2 &&) -> JOIN_ENV<std::remove_cvref_t<Env1>, std::remove_cvref_t<Env2>>;

}; // namespace mcs::execution::snd::general