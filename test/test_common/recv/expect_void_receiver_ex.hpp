#pragma once

#include "./base_expect_receiver.hpp"
#include "boost/ut.hpp"

namespace test
{
    template <class Env = mcs::execution::empty_env>
    struct expect_void_receiver_ex : base_expect_receiver<Env>
    {

        explicit expect_void_receiver_ex(bool &executed) : m_executed(&executed) {}

        template <class... Ty>
        void set_value(const Ty &.../*unused*/) noexcept // NOLINT
        {
            *m_executed = true;
        }

        void set_stopped() noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_stopped called on expect_void_receiver_ex";
        }

        void set_error(std::exception_ptr /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_error called on expect_void_receiver_ex";
        }

      private:
        bool *m_executed;
    };
}; // namespace test