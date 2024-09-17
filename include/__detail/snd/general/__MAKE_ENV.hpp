#pragma once

#include <utility>

namespace mcs::execution::snd::general
{
    template <typename Q, typename V>
    class MAKE_ENV // NOLINT
    {
        V value; // NOLINT

      public:
        template <typename _V>
        MAKE_ENV(Q && /*unused*/, _V &&value) : value(std::forward<_V>(value))
        {
        }

        // env.query(q) refers remains valid while env remains valid
        constexpr auto query(Q const & /*unused*/) const -> V const &
        {
            return this->value;
        }
        constexpr auto query(Q const & /*unused*/) -> V &
        {
            return this->value;
        }
    };

    template <typename Q, typename V>
    MAKE_ENV(Q &&, V &&value) -> MAKE_ENV<std::remove_cvref_t<Q>, std::remove_cvref_t<V>>;

}; // namespace mcs::execution::snd::general