#pragma once
#include "./__declarations.hpp"

namespace mcs::execution::stoptoken
{
    // The class inplace_stop_source models stoppable-source.
    class inplace_stop_source
    {
      public:
        // [stopsource.inplace.cons], constructors, copy, and assignment
        // Effects: Initializes a new stop state inside *this.
        // Postconditions: stop_requested() is false
        constexpr inplace_stop_source() noexcept;

        inplace_stop_source(inplace_stop_source &&) = delete;
        inplace_stop_source(const inplace_stop_source &) = delete;
        inplace_stop_source &operator=(inplace_stop_source &&) = delete;
        inplace_stop_source &operator=(const inplace_stop_source &) = delete;
        ~inplace_stop_source();

        //[stopsource.inplace.mem], stop handling
        // Returns: A new associated inplace_stop_token object.
        // The inplace_stop_token object’s stop-source member is equal to this.
        constexpr inplace_stop_token get_token() const noexcept; // NOLINT
        static constexpr bool stop_possible() noexcept           // NOLINT
        {
            return true;
        }
        // Returns:
        // true if the stop state inside *this has received a stop request;
        // otherwise, false.
        bool stop_requested() const noexcept; // NOLINT
        // Effects: Executes a stop request operation ([stoptoken.concepts]).
        // Postconditions: stop_requested() is true.
        bool request_stop() noexcept; // NOLINT
    };
}; // namespace mcs::execution::stoptoken