#pragma once

#include <tuple>

namespace mcs::execution::cmplsigs::__detail
{
    template <template <typename> class Predicate, typename Tuple>
    struct filter_tuple;

    template <template <typename> class Predicate, typename... Ts>
    struct filter_tuple<Predicate, std::tuple<Ts...>>
    {
        using type = decltype(std::tuple_cat(
            std::declval<std::conditional_t<Predicate<Ts>::value, std::tuple<Ts>,
                                            std::tuple<>>>()...));
    };

}; // namespace mcs::execution::cmplsigs::__detail