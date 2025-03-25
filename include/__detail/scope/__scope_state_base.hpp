#pragma once

namespace mcs::execution
{
    struct scope_state_base
    {
        using callback_fun_t = void (*)(scope_state_base *self) noexcept;
        callback_fun_t execute{};
        scope_state_base *next{};
    };

}; // namespace mcs::execution