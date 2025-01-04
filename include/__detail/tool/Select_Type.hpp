#pragma once

#include <type_traits>

namespace mcs::execution::tool
{
    template <template <typename> class Predicate, typename Type>
    struct Select_Type;

    template <template <typename...> class Template, template <typename> class Predicate,
              typename... T>
    struct Select_Type<Predicate, Template<T...>>
    {
        template <typename Rest, typename Collect>
        struct Select;

        template <typename... Added>
        struct Select<Template<>, Template<Added...>>
        {
            using type = Template<Added...>;
        };

        template <typename Cur, typename... Rest, typename... Added>
        struct Select<Template<Cur, Rest...>, Template<Added...>>
        {
            using type = std::conditional_t<
                Predicate<Cur>::value,
                typename Select<Template<Rest...>, Template<Added..., Cur>>::type,
                typename Select<Template<Rest...>, Template<Added...>>::type>;
        };

        using type = typename Select<Template<T...>, Template<>>::type;
    };
}; // namespace mcs::execution::tool