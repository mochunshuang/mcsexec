#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"

namespace test
{
    template <class F>
    struct fun_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;
        F f; // NOLINT

        template <class... Ts>
        void set_value(Ts... vals) noexcept // NOLINT
        {
            try
            {
                std::move(f)(static_cast<Ts &&>(vals)...);
            }
            catch (...)
            {
                mcs::execution::recv::set_error(std::move(*this),
                                                std::current_exception());
            }
        }

        void set_stopped() noexcept // NOLINT
        {
            UNEXPECT("Done called");
        }

        void set_error(std::exception_ptr eptr) noexcept // NOLINT
        {
            try
            {
                if (eptr)
                    std::rethrow_exception(eptr);
                UNEXPECT("Empty exception thrown");
            }
            catch (const std::exception &e)
            {
                UNEXPECT("Exception thrown: " << e.what());
            }
        }
    };

    template <class F>
    fun_receiver<F> make_fun_receiver(F f)
    {
        return fun_receiver<F>{std::forward<F>(f)};
    }
}; // namespace test