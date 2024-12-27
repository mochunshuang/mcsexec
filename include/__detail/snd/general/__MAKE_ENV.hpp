#pragma once

#include <type_traits>
#include <utility>

namespace mcs::execution::snd::general
{
    template <typename Q, typename V>
    class MAKE_ENV // NOLINT
    {
        V value; // NOLINT

      public:
        template <typename _Q, typename _V>
        MAKE_ENV(_Q && /*unused*/, _V &&value) : value(std::forward<_V>(value))
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
    MAKE_ENV(Q &&, V &&value) -> MAKE_ENV<std::decay_t<Q>, std::decay_t<V>>;

}; // namespace mcs::execution::snd::general