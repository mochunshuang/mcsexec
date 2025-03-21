#pragma once

#include "../cmplsigs/__completion_signatures.hpp"
#include <exception>
#include <variant>

namespace mcs::execution::scope
{
    namespace __detail
    {
        template <class Tag, class... Ts> // NOLINTNEXTLINE
        inline static consteval bool has_set_error_ptr(Tag (*)(Ts...))
        {
            return std::is_same_v<Tag(Ts...), set_error_t(std::exception_ptr)>;
        }
        template <class Tag, class... Ts> // NOLINTNEXTLINE
        inline static consteval auto check_tuple(Tag (*)(Ts...)) noexcept(
            std::is_nothrow_constructible_v<decayed_tuple<Tag, Ts...>, std::decay_t<Tag>,
                                            std::decay_t<Ts>...>)
        {
            return decayed_tuple<Tag, Ts...>{};
        }
    }; // namespace __detail

    template <class Sigs>
    struct spawn_future_state_base
    {
        template <class... Sig>
        static consteval auto get_variant( // NOLINT
            cmplsigs::completion_signatures<Sig...> /*unused*/)
        {
            constexpr bool has_epr = // NOLINT
                (__detail::has_set_error_ptr(static_cast<Sig *>(nullptr)) || ...);
            constexpr bool tuple_nothrow = // NOLINT
                (noexcept(__detail::check_tuple(static_cast<Sig *>(nullptr))) && ...);
            if constexpr (has_epr || tuple_nothrow)
            {
                using T =
                    std::variant<std::monostate, decltype(__detail::check_tuple(
                                                     static_cast<Sig *>(nullptr)))...>;
                return static_cast<T *>(nullptr);
            }
            else
            {
                using T = std::variant<std::monostate,
                                       decltype(__detail::check_tuple(
                                           static_cast<Sig *>(nullptr)))...,
                                       decayed_tuple<set_error_t, std::exception_ptr>>;
                return static_cast<T *>(nullptr);
            }
        };

        std::remove_pointer_t<decltype(get_variant(Sigs{}))> result; // NOLINT
        virtual void complete() = 0;                                 // NOLINT
    };
}; // namespace mcs::execution::scope