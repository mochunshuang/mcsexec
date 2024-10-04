#pragma once

#include <memory>

#include "./__stop_token.hpp"
#include "./__nostopstate_t.hpp"

namespace mcs::execution::stoptoken
{
    // stop_source models stoppable-source, copyable, equality_comparable, and
    // swappable
    class stop_source // NOLINT
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
        stop_token get_token() const noexcept; // NOLINT

        bool stop_possible() const noexcept; // NOLINT

        bool stop_requested() const noexcept; // NOLINT

        bool request_stop() noexcept; // NOLINT

        bool operator==(const stop_source &rhs) const noexcept = default;

      private:
        using unspecified = ::mcs::execution::stoptoken::stop_state;
        // stop-state refers to the stop_source's associated stop state.
        // A stop_source object is disengaged when stop-state is empty.
        std::shared_ptr<unspecified> stop_state{nullptr}; // NOLINT
    };

}; // namespace mcs::execution::stoptoken