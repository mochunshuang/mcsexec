#pragma once

#include "./__stoppable_token.hpp"

namespace mcs::execution::stoptoken
{
    template <class Source>
    concept stoppable_source = requires(
        Source &src,       // NOLINTNEXTLINE
        const Source csrc) // see implicit expression variations ([concepts.equality])
    {
        { csrc.get_token() } -> stoppable_token;
        { csrc.stop_possible() } noexcept -> std::same_as<bool>;
        { csrc.stop_requested() } noexcept -> std::same_as<bool>;
        { src.request_stop() } -> std::same_as<bool>;
    }; // exposition only

}; // namespace mcs::execution::stoptoken
