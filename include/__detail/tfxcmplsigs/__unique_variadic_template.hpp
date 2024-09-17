#pragma once

#include <concepts>
#include <type_traits>

namespace mcs::execution::tfxcmplsigs
{
    namespace __detail
    {
        template <typename T, typename... S>
        concept left_contains = (std::same_as<T, S> || ...); // NOLINT

        template <typename T, typename U>
        struct __unique_variadic_template_impl;

        template <template <class...> class Tuple, typename T, typename... Ts,
                  typename... As>
        struct __unique_variadic_template_impl<Tuple<T, Ts...>, Tuple<As...>>
        {
            using type = __unique_variadic_template_impl<   //
                Tuple<Ts...>,                               //
                std::conditional_t<left_contains<T, As...>, //
                                   Tuple<As...>, Tuple<As..., T>>>::type;
        };

        template <template <class...> class Tuple, typename... As>
        struct __unique_variadic_template_impl<Tuple<>, Tuple<As...>>
        {
            using type = Tuple<As...>;
        };
    }; // namespace __detail

    template <typename T>
    struct unique_variadic_template;
    template <template <class...> class Tuple, typename... As>
    struct unique_variadic_template<Tuple<As...>>
    {
        using type =
            __detail::__unique_variadic_template_impl<Tuple<As...>, Tuple<>>::type;
    };

}; // namespace mcs::execution::tfxcmplsigs