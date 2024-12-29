#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"
#include <any>
#include <utility>

namespace test
{
    struct any_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;
        bool *called;   // NOLINT
        std::any *data; // NOLINT

        template <typename... A> // NOLINTNEXTLINE
        auto set_value(A &&...a) && noexcept -> void
        {
            *this->called = true;
            if constexpr (sizeof...(A) > 0)
            {
                *this->data = std::make_tuple(std::forward<A>(a)...);
            }
        }

        template <typename E> // NOLINTNEXTLINE
        auto set_error(E &&e) && noexcept -> void
        {
            *this->called = true;
            *this->data = std::forward<E>(e);
        }

        void set_stopped() && noexcept // NOLINT
        {
            *this->called = true;
        }

        auto &refData()
        {
            return data;
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return mcs::execution::empty_env{};
        }
    };
}; // namespace test