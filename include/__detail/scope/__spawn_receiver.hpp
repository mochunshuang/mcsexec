#pragma once

#include "./__spawn_state_base.hpp"
#include ".././__core_types.hpp"

namespace mcs::execution::scope
{
    struct spawn_receiver
    {
        using receiver_concept = receiver_t;
        spawn_state_base *state; // NOLINT

        void set_value() && noexcept // NOLINT
        {
            state->complete(state);
        }
        void set_stopped() && noexcept // NOLINT
        {
            state->complete(state);
        }
    };
}; // namespace mcs::execution::scope
