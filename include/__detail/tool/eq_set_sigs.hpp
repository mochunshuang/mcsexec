#pragma once

#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::tool
{
    template <typename T0, typename T1>
    struct is_one_of;

    template <typename Sig, typename... T>
    struct is_one_of<Sig, cmplsigs::completion_signatures<T...>>
    {
        static constexpr bool value = (std::same_as<Sig, T> || ...); // NOLINT
    };

    // Note: T0 or T1 mush unique, no repeat
    namespace __detail
    {
        template <typename T0, typename T1>
        struct eq_set_sigs;

        template <typename... T0, typename... T1>
            requires(not(sizeof...(T0) == sizeof...(T1)))
        struct eq_set_sigs<cmplsigs::completion_signatures<T0...>,
                           cmplsigs::completion_signatures<T1...>>
        {
            static constexpr bool value = false; // NOLINT
        };

        template <typename... T0, typename... T1>
            requires(sizeof...(T0) == sizeof...(T1))
        struct eq_set_sigs<cmplsigs::completion_signatures<T0...>,
                           cmplsigs::completion_signatures<T1...>>
        {
            using Set0 = cmplsigs::completion_signatures<T0...>;
            using Set1 = cmplsigs::completion_signatures<T1...>;
            static constexpr bool all_in_Set0 = // NOLINT
                (is_one_of<T1, Set0>::value && ...);
            static constexpr bool all_in_Set1 = // NOLINT
                (is_one_of<T0, Set1>::value && ...);
            static constexpr bool value = all_in_Set0 && all_in_Set1; // NOLINT
        };
    }; // namespace __detail

    template <typename T0, typename T1>
    constexpr bool eq_set_sigs_v = __detail::eq_set_sigs<T0, T1>::value; // NOLINT

    template <typename T0, typename T1>
    constexpr bool is_same_v = eq_set_sigs_v<T0, T1>; // NOLINT

}; // namespace mcs::execution::tool