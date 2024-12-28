#pragma once

#include "../../../include/execution.hpp"
#include <exception>

namespace test
{
    struct recv0_ec
    {
        using receiver_concept = mcs::execution::receiver_t;

        void set_value() noexcept {} // NOLINT

        void set_stopped() noexcept {} // NOLINT

        void set_error(std::exception_ptr) noexcept {} // NOLINT

        void set_error(std::error_code) noexcept {} // NOLINT
    };
}; // namespace test
