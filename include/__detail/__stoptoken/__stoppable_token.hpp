#pragma once

#include <concepts>

namespace mcs::execution::stoptoken
{
    //////////////////////////////////////////////////////////////////////////////
    // [exec.stop_token.concepts]
    namespace __detail
    {
        template <typename T, typename = void>
        struct __has_callback_type : std::false_type
        {
        };
        template <typename T>
        struct __has_callback_type<T,
                                   std::void_t<typename T::template callback_type<void>>>
            : std::true_type
        {
        };
        template <typename T>
        concept has_callback_type = __has_callback_type<T>::value;
    }; // namespace __detail

    template <class Token>
    concept stoppable_token = __detail::has_callback_type<Token> &&
                              requires(const Token tok) { // NOLINT
                                  { tok.stop_requested() } noexcept -> std::same_as<bool>;
                                  { tok.stop_possible() } noexcept -> std::same_as<bool>;
                                  {
                                      Token(tok)
                                  } noexcept; // see implicit expression variations
                                              // ([concepts.equality])
                              } && std::copyable<Token> &&
                              std::equality_comparable<Token> && std::swappable<Token>;

}; // namespace mcs::execution::stoptoken
