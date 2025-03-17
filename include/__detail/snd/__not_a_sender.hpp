#pragma once
#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution
{
    struct not_a_sender
    {
        using sender_concept = sender_t;

        auto get_completion_signatures(auto &&) const // NOLINT
        {
        }
        template <class Sndr>
        static constexpr auto get_completion_signatures() // NOLINT
            -> cmplsigs::completion_signatures<>
        {
            struct not_completion_signatures
            {
            };
            throw not_completion_signatures{};
        }
    };
}; // namespace mcs::execution