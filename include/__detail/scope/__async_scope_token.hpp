#pragma once

#include <concepts>
#include "../snd/__sender_in.hpp"

namespace mcs::execution::scope
{
    using test_env = empty_env;
    struct test_sender
    {
        using sender_concept = sender_t;
        auto get_env() noexcept // NOLINT
        {
            return test_env{};
        }
        using completion_signatures = cmplsigs::completion_signatures<set_value_t()>;
    };
    static_assert(snd::sender<test_sender> && queryable<test_env>);
    static_assert(snd::sender_in<test_sender>);
    static_assert(snd::sender_in<test_sender, empty_env>);

    // NOTE: async_scope_token<Token> is modeled only if Token’s copy operations, move
    // operations, and disassociate method do not exit with an exception.
    // NOTE: do not change completion_signatures by sndr
    template <class Token>
    concept async_scope_token = std::copyable<Token> && requires(Token token) {
        { token.try_associate() } noexcept -> std::same_as<bool>;
        { token.disassociate() } noexcept -> std::same_as<void>;
        { token.wrap(std::declval<test_sender>()) } noexcept -> snd::sender_in<test_env>;
    };
}; // namespace mcs::execution::scope