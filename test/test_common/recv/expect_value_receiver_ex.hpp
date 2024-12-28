#pragma once

#include "../../../include/execution.hpp"
#include <boost/ut.hpp>

namespace test
{

    template <class T, class Env = mcs::execution::empty_env>
    class expect_value_receiver_ex // NOLINT
    {
        T *m_dest;
        Env m_env{};

      public:
        using receiver_concept = mcs::execution::receiver_t;

        explicit expect_value_receiver_ex(T &dest) : m_dest(&dest) {}

        expect_value_receiver_ex(Env env, T &dest) : m_dest(&dest), m_env(std::move(env))
        {
        }

        void set_value(T val) noexcept // NOLINT
        {
            *m_dest = val;
        }

        template <class... Ts>
        void set_value(Ts... /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false)
                << "set_value called with wrong value types on expect_value_receiver_ex";
        }

        void set_stopped() noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_stopped called on expect_value_receiver_ex";
        }

        void set_error(std::exception_ptr /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_error called on expect_value_receiver_ex";
        }

        Env get_env() const noexcept // NOLINT
        {
            return m_env;
        }
    };

}; // namespace test
