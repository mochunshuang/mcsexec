#pragma once

#include <concepts>
namespace mcs::execution::snd::general
{
    namespace __detail
    {
        template <typename Fun>
        struct matching_sig
        {
            using type = Fun;
        };
        template <typename Return, typename... Args>
        struct matching_sig<Return(Args...)>
        {
            using type = Return(Args &&...);
        };
    }; // namespace __detail

    template <typename Fun1, typename Fun2>
    inline constexpr bool MATCHING_SIG = // NOLINT
        std::same_as<typename __detail::matching_sig<Fun1>::type,
                     typename __detail::matching_sig<Fun2>::type>;

}; // namespace mcs::execution::snd::general