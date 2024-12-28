#pragma once

#include "./base_expect_receiver.hpp"
#include "./env_tag.hpp"

namespace test
{

    template <class Env = mcs::execution::empty_env, class... Ts>
    struct expect_value_receiver : base_expect_receiver<Env>
    {

        explicit(sizeof...(Ts) != 1) expect_value_receiver(Ts... vals)
            : m_values(std::move(vals)...)
        {
        }

        expect_value_receiver(env_tag /*unused*/, Env env, Ts... vals)
            : base_expect_receiver<Env>(std::move(env)), m_values(std::move(vals)...)
        {
        }

        void set_value(const Ts &...vals) noexcept // NOLINT
        {
            boost::ut::expect(m_values == std::tie(vals...));
            this->set_called();
        }

        template <class... Us>
        void set_value(const Us &.../*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false)
                << "set_value called with wrong value types on expect_value_receiver";
        }

        void set_stopped() noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_stopped called on expect_value_receiver";
        }

        template <class E>
        void set_error(E /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_error called on expect_value_receiver";
        }

      private:
        std::tuple<Ts...> m_values;
    };
}; // namespace test