#pragma once

namespace mcs::execution::scope
{
    struct spawn_state_base
    {
        spawn_state_base(const spawn_state_base &) = delete;
        spawn_state_base(spawn_state_base &&) = delete;
        spawn_state_base &operator=(const spawn_state_base &) = delete;
        spawn_state_base &operator=(spawn_state_base &&) = delete;

        spawn_state_base() = default;
        virtual ~spawn_state_base() = default;
        virtual void complete() = 0; // exposition-only
    };
}; // namespace mcs::execution::scope
