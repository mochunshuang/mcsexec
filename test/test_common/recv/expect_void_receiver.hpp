#pragma once
#include "./base_expect_receiver.hpp"
#include "boost/ut.hpp"

namespace test
{
    template <class Env = mcs::execution::empty_env>
    struct expect_void_receiver : base_expect_receiver<Env>
    {
        expect_void_receiver() = default;

        explicit expect_void_receiver(Env env) : base_expect_receiver<Env>(std::move(env))
        {
        }

        void set_value() noexcept // NOLINT
        {
            this->set_called();
        }

        template <class... Ts>
        void set_value(Ts...) noexcept // NOLINT
        {
            boost::ut::expect(false)
                << "set_value called on expect_void_receiver with some non-void value";
        }

        void set_stopped() noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_stopped called on expect_void_receiver";
        }

        void set_error(std::exception_ptr) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_error called on expect_void_receiver";
        }
    };
}; // namespace test