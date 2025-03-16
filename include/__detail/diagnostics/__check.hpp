#pragma once
#include "../cmplsigs/__completion_signatures.hpp"
#include "./__err_msg.hpp"

namespace mcs::execution
{
    namespace diagnostics
    {

        struct universal_arg
        {
            template <typename T>
            consteval operator T() const noexcept; // NOLINT
        };

        template <typename F>
        concept check_set_stoped_arg = requires(F &&f) { static_cast<F &&>(f)(); };
        template <typename F>
        concept check_set_error_arg = requires(F &&f, universal_arg &&e) {
            static_cast<F &&>(f)(static_cast<universal_arg &&>(e));
        };

        template <class... T>
        inline constexpr bool check_type_impl = false; // NOLINT

        template <typename... Ts>
        concept check_type = check_type_impl<Ts...>;

        // NOLINTBEGIN
        template <class... What>
        struct sender_type_check_failure
        {
            template <class... Info>
            consteval explicit sender_type_check_failure(Info &&.../*unused*/)
            {
            }
        };
        // NO defined replace throw
        template <class... What, class... Info>
        [[noreturn, nodiscard]] consteval cmplsigs::completion_signatures<>
        invalid_completion_signature(Info &&...info);
    }; // namespace diagnostics

}; // namespace mcs::execution