#pragma once

namespace mcs::execution::scope
{
    struct spawn_state_base
    {
        using callback_fun_t = void (*)(spawn_state_base *self) noexcept;
        callback_fun_t complete{};
    };
}; // namespace mcs::execution::scope
