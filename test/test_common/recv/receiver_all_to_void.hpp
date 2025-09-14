#pragma once

#include "../../../include/execution.hpp"

namespace test
{
    struct receiver_all_to_void
    {
        using receiver_concept = mcs::execution::receiver_t;

        template <typename... A> // NOLINTNEXTLINE
        auto set_value(A &&...a) && noexcept -> void
        {
        }

        template <typename E> // NOLINTNEXTLINE
        auto set_error(E &&) && noexcept -> void
        {
        }

        void set_stopped() && noexcept // NOLINT
        {
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return ex::empty_env{};
        }
    };
}; // namespace test