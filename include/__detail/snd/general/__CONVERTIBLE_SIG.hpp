#pragma once

#include <concepts>

namespace mcs::execution::snd::general
{
    namespace __detail
    {
        template <typename F1, typename F2>
        inline constexpr bool convertible_args = false; // NOLINT

        template <typename R1, typename R2, typename... Args1, typename... Args2>
            requires(sizeof...(Args1) == sizeof...(Args2)) &&
                        ((std::is_same_v<std::decay_t<Args1>, std::decay_t<Args2>> &&
                          std::is_convertible_v<Args1, Args2>) &&
                         ...) // NOLINTNEXTLINE
        inline constexpr bool convertible_args<R1(Args1...), R2(Args2...)> = true;

        template <typename F1, typename F2>
        struct convertible_sig
        {
            static constexpr bool value = false; // NOLINT
        };
        template <typename R1, typename R2, typename... Args1, typename... Args2>
            requires std::same_as<R1, R2> && convertible_args<R1(Args1...), R2(Args2...)>
        struct convertible_sig<R1(Args1...), R2(Args2...)>
        {
            static constexpr bool value = true; // NOLINT
        };
    }; // namespace __detail

    template <typename From, typename To>
    inline constexpr bool CONVERTIBLE_SIG = // NOLINT
        __detail::convertible_sig<From, To>::value;

    template <typename List, typename To>
    inline constexpr bool HAS_CONVERTIBLE_SIG = false; // NOLINT

    template <template <typename...> typename List, typename To, typename... From>
        requires(__detail::convertible_args<From, To> || ...)
    inline constexpr bool HAS_CONVERTIBLE_SIG<List<From...>, To> = true; // NOLINT

}; // namespace mcs::execution::snd::general
