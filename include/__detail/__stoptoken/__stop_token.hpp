#pragma once

#include "./__declarations.hpp"

#include <memory>

namespace mcs::execution::stoptoken
{
    // [stoptoken.general]
    // The class stop_token models the concept stoppable_token
    class stop_token
    {
      public:
        template <class CallbackFn>
        using callback_type = stop_callback<CallbackFn>;

        stop_token() noexcept = default;

        // [stoptoken.mem], Member functions
        // Equivalent to: stop_state.swap(rhs.stop_state)
        void swap(stop_token &) noexcept;

        // true if this stop_token has received a stop request; otherwise, false.
        bool stop_requested() const noexcept; // NOLINT
        // false if *this is disengaged ,
        //  or a stop request was not made and there are no associated stop_source objects
        //  otherwise, true.
        bool stop_possible() const noexcept; // NOLINT

        bool operator==(const stop_token &rhs) const noexcept = default;

      private:
        using unspecified = int;
        // stop-state refers to the stop_token's associated stop state.
        // A stop_token object is disengaged when stop-state is empty.
        std::shared_ptr<unspecified> stop_state; // NOLINT // exposition only
    };

}; // namespace mcs::execution::stoptoken