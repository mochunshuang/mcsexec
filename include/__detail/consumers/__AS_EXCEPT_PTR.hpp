#pragma once

#include <exception>
#include <system_error>

namespace mcs::execution::consumers
{
    template <typename Err>
    std::exception_ptr AS_EXCEPT_PTR(Err &&err)
    {
        using DecayedErr = std::decay_t<Err>;

        if constexpr (std::is_same_v<DecayedErr, std::exception_ptr>)
        {
            // Mandates: err != exception_ptr() is true.
            // static_assert(err != std::exception_ptr(),
            //               "err must not be a null exception_ptr");
            assert(err != std::exception_ptr() && "err must not be a nullexception_ptr");
            return err;
        }
        else if constexpr (std::is_same_v<DecayedErr, std::error_code>)
        {
            return std::make_exception_ptr(std::system_error(err));
        }
        else
        {
            return std::make_exception_ptr(std::forward<Err>(err));
        }
    }
}; // namespace mcs::execution::consumers