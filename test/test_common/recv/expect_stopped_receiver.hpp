#pragma once

#include "./base_expect_receiver.hpp"

namespace test
{
    template <class Env = mcs::execution::empty_env>
    struct expect_stopped_receiver : base_expect_receiver<Env>
    {
        expect_stopped_receiver() = default;

        explicit expect_stopped_receiver(Env env)
            : base_expect_receiver<Env>(std::move(env))
        {
        }

        template <class... Ts>
        void set_value(Ts... /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_value called on expect_stopped_receiver";
        }

        void set_stopped() noexcept // NOLINT
        {
            this->set_called();
        }

        void set_error(std::exception_ptr /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_error called on expect_stopped_receiver";
        }
    };
}; // namespace test