#pragma once

#include "../../../include/execution.hpp"
#include <boost/ut.hpp>

namespace test
{
    template <class Env = mcs::execution::empty_env>
    struct expect_stopped_receiver_ex
    {
        using receiver_concept = mcs::execution::receiver_t;

        explicit expect_stopped_receiver_ex(bool &executed) : m_executed(&executed) {}

        expect_stopped_receiver_ex(Env env, bool &executed)
            : m_executed(&executed), m_env(std::move(env))
        {
        }

        template <class... Ts>
        void set_value(Ts... /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_value called on expect_stopped_receiver_ex";
        }

        void set_stopped() noexcept // NOLINT
        {
            *m_executed = true;
        }

        void set_error(std::exception_ptr /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_error called on expect_stopped_receiver_ex";
        }

        Env get_env() const noexcept // NOLINT
        {
            return m_env;
        }

      private:
        bool *m_executed;
        Env m_env;
    };
}; // namespace test