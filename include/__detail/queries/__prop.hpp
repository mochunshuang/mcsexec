#pragma once

#include <type_traits>

namespace mcs::execution::queries
{

    /**
     * @brief Class template prop is for building a queryable object from a query object
     * and a value.
     * Mandates: callable<QueryTag, prop-like<ValueType>> is modeled
     *
     *   template <class ValueType>
     *   struct prop_like
     *   {
     *       const ValueType &query(auto) const noexcept;
     *   };
     * @tparam ValueType
     */
    template <class QueryTag, class ValueType>
    struct prop
    {
        QueryTag q;  // exposition only // NOLINT
        ValueType v; // exposition only // NOLINT

        constexpr const ValueType &query(QueryTag /*unused*/) const noexcept
        {
            return v;
        }
    };

    template <class QueryTag, class ValueType>
    prop(QueryTag, ValueType) -> prop<QueryTag, std::unwrap_reference_t<ValueType>>;

}; // namespace mcs::execution::queries