#pragma once

#include <concepts>

namespace mcs::execution::stoptoken
{
    //////////////////////////////////////////////////////////////////////////////
    // [exec.stop_token.concepts]
    namespace __detail
    {
        template <template <typename> typename>
        struct check_type_alias_exists;
    }; // namespace __detail

    template <class Token>
    concept stoppable_token = requires(const Token tok) { // NOLINT
        typename __detail::check_type_alias_exists<Token::template callback_type>;
        { tok.stop_requested() } noexcept -> std::same_as<bool>;
        { tok.stop_possible() } noexcept -> std::same_as<bool>;
        {
            Token(tok)
        } noexcept; // see implicit expression variations
                    // ([concepts.equality])
    } && std::copyable<Token> && std::equality_comparable<Token> && std::swappable<Token>;

}; // namespace mcs::execution::stoptoken
