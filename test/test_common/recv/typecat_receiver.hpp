#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"

namespace test
{
    enum class typecat // NOLINT
    {
        undefined, // NOLINT
        value,     // NOLINT
        ref,       // NOLINT
        cref,      // NOLINT
        rvalref,   // NOLINT
    };

    template <class T>
    struct typecat_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;
        T *value;     // NOLINT
        typecat *cat; // NOLINT

        void set_value(T &v) noexcept // NOLINT
        {
            *value = v;
            *cat = typecat::ref;
        }

        void set_value(const T &v) noexcept // NOLINT
        {
            *value = v;
            *cat = typecat::cref;
        }

        void set_value(T &&v) noexcept // NOLINT
        {
            *value = v;
            *cat = typecat::rvalref;
        }

        void set_stopped() noexcept // NOLINT
        {
            UNEXPECT("set_stopped called");
        }

        void set_error(std::exception_ptr) noexcept // NOLINT
        {
            UNEXPECT("set_error called");
        }
    };

}; // namespace test