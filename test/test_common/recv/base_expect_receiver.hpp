#pragma once

#include "../../../include/execution.hpp"
#include <boost/ut.hpp>

namespace test
{
    template <class Env = mcs::execution::empty_env>
    class base_expect_receiver // NOLINT
    {
        std::atomic<bool> m_called{false};
        Env m_env{};

      public:
        using receiver_concept = mcs::execution::receiver_t;
        base_expect_receiver() = default;

        ~base_expect_receiver()
        {
            boost::ut::expect(m_called.load());
        }

        explicit base_expect_receiver(Env env) : m_env(std::move(env)) {}

        // Note: 可能是bug的来源。 被move过就是 called. 语义有问题
        base_expect_receiver(base_expect_receiver &&other) noexcept
            : m_called(other.m_called.exchange(true)), m_env(std::move(other.m_env))
        {
        }

        base_expect_receiver &operator=(base_expect_receiver &&other) = delete;

        base_expect_receiver(const base_expect_receiver &other) noexcept
            : m_called(other.m_called.load()), m_env(std::move(other.m_env)) {};
        base_expect_receiver &operator=(const base_expect_receiver &other) = default;

        void set_called() // NOLINT
        {
            m_called.store(true);
        }

        bool is_called() const // NOLINT
        {
            return m_called.load();
        }

        Env get_env() const noexcept // NOLINT
        {
            return m_env;
        }
    };
}; // namespace test