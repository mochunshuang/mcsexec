#pragma once

#include "./__declarations.hpp"
#include "./__nostopstate_t.hpp"
#include <memory>

namespace mcs::execution::stoptoken
{
    // stop_source models stoppable-source, copyable, equality_comparable, and swappable
    class stop_source
    {
      public:
        // 33.3.4.2, constructors, copy, and assignment
        // Effect: Initialises stop-state with a pointer to a new stop state.
        // Postconditions: stop_possible() is true and stop_requested() is false.
        // Throws: bad_alloc if memory cannot be allocated for the stop state.
        stop_source();

        explicit stop_source(nostopstate_t /*unused*/) {}

        // [stopsource.mem], Member functions
        // Effects: Equivalent to: stop-state.swap(rhs.stop-state) .
        void swap(stop_source &) noexcept;

        // 33.3.4.3, stop handling
        // Returns:
        //  stop_token() if stop_possible() is false;
        //  otherwise a new associated stop_token object ;
        //      i.e., its stop-state member is equal to the stop-state member of *this
        stop_token get_token() const noexcept; // NOLINT
        // Returns: stop-state != nullptr .
        bool stop_possible() const noexcept; // NOLINT
        // Returns:
        // true if stop-state refers to a stop state that has received a stop request;
        // otherwise, false.
        bool stop_requested() const noexcept; // NOLINT
        // Effects: Executes a stop request operation ([stoptoken.concepts]) on the
        // associated stop state
        bool request_stop() noexcept; // NOLINT

        bool operator==(const stop_source &rhs) const noexcept = default;

      private:
        using unspecified = int;
        // stop-state refers to the stop_source's associated stop state.
        // A stop_source object is disengaged when stop-state is empty.
        std::shared_ptr<unspecified> stop_state; // NOLINT
    };

}; // namespace mcs::execution::stoptoken