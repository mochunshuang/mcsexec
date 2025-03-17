#pragma once

#include "../snd/__sender.hpp"

#include "../snd/general/__FWD_ENV.hpp"
#include "../snd/__detail/mate_type/__child_type.hpp"
#include "../snd/__get_completion_signatures.hpp"

namespace mcs::execution::diagnostics
{
    template <class... Fns>
    struct overload_set : Fns...
    {
        using Fns::operator()...;
    };

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

    template <snd::sender Sndr, class... Env> // NOLINTNEXTLINE
    inline constexpr bool check_type_impl<Sndr, Env...> = []() consteval {
        using index = Sndr::indices_for;
        []<std::size_t... Is>(std::index_sequence<Is...>) {
            (static_cast<void>(
                 snd::get_completion_signatures<
                     snd::__detail::mate_type::child_type<Sndr, Is>,
                     decltype(snd::general::FWD_ENV(std::declval<Env>()))...>()),
             ...);
        }(index{});
        return true;
    }();

    template <typename... Ts>
    concept check_type = check_type_impl<Ts...>;

}; // namespace mcs::execution::diagnostics