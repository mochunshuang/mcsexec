#pragma once

#include "./__stoppable_token.hpp"

namespace mcs::execution::stoptoken
{
    template <class Source>
    concept stoppable_source = requires(
        Source &src,       // NOLINTNEXTLINE
        const Source csrc) // see implicit expression variations ([concepts.equality])
    {
        //  check for p3409r1 late. Still needed get_token() or get_token<Idx>()
        // { csrc.get_token() } -> stoppable_token;
        { csrc.stop_possible() } noexcept -> std::same_as<bool>;
        { csrc.stop_requested() } noexcept -> std::same_as<bool>;
        { src.request_stop() } -> std::same_as<bool>;
        // Still needed get_token() or get_token<Idx>()
        requires requires {
            { csrc.get_token() } -> stoppable_token;
        } || requires {
            // finite_inplace_stop_token; std::size_t{}==0
            { csrc.template get_token<std::size_t{}>() } -> stoppable_token;
        };
    }; // exposition only

}; // namespace mcs::execution::stoptoken
