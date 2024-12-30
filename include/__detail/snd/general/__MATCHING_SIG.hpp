#pragma once

#include <concepts>
namespace mcs::execution::snd::general
{
    namespace __detail
    {
        template <typename F1, typename F2>
        struct matching_sig
        {
            static constexpr bool value = false; // NOLINT
        };
        template <typename R1, typename R2, typename... Args1, typename... Args2>
            requires std::same_as<R1(Args1 &&...), R2(Args2 &&...)>
        struct matching_sig<R1(Args1...), R2(Args2...)>
        {
            static constexpr bool value = true; // NOLINT
        };
    }; // namespace __detail

    template <typename F1, typename F2>
    inline constexpr bool MATCHING_SIG = // NOLINT
        __detail::matching_sig<F1, F2>::value;

}; // namespace mcs::execution::snd::general