#pragma once

#include "../../../include/execution.hpp"
#include <exception>

namespace test
{
    struct recv_int
    {
        using receiver_concept = mcs::execution::receiver_t;

        void set_value(int) noexcept {} // NOLINT

        void set_stopped() noexcept {} // NOLINT

        void set_error(std::exception_ptr) noexcept {} // NOLINT
    };
}; // namespace test
