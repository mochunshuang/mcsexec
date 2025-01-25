#pragma once

namespace mcs::execution::stoptoken
{
    struct inplace_callback_base
    {
        using callback_fun_t = void(inplace_callback_base *self) noexcept;
        inplace_callback_base *next{}; // NOLINT
        callback_fun_t *invoke_callback{};
    };

}; // namespace mcs::execution::stoptoken