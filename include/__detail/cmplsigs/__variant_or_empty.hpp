#pragma once
#include <variant>

namespace mcs::execution::cmplsigs
{
    struct empty_variant
    {
        empty_variant() = delete; // exposition only
    };

    namespace __detail
    {
        template <bool IsEmpty, typename... Ts>
        struct __variant_or_empty;

        template <typename... Ts>
        struct __variant_or_empty<true, Ts...>
        {
            using type = std::variant<std::decay_t<Ts>...>;
        };

        template <typename... Ts>
        struct __variant_or_empty<false, Ts...>
        {
            using type = empty_variant;
        };
    }; // namespace __detail

    template <class... Ts>
    using variant_or_empty =
        __detail::__variant_or_empty<(sizeof...(Ts) > 0), Ts...>::type;

}; // namespace mcs::execution::cmplsigs