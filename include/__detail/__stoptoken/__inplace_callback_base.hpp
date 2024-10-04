#pragma once

namespace mcs::execution::stoptoken
{
    struct inplace_callback_base
    {
        inplace_callback_base *next{}; // NOLINT

        virtual ~inplace_callback_base() = default;
        virtual auto invoke_callback() -> void = 0; // NOLINT

        inplace_callback_base() = default;
        inplace_callback_base(inplace_callback_base &&) = default;
        inplace_callback_base(const inplace_callback_base &) = default;
        inplace_callback_base &operator=(inplace_callback_base &&) = default;
        inplace_callback_base &operator=(const inplace_callback_base &) = default;
    };

}; // namespace mcs::execution::stoptoken