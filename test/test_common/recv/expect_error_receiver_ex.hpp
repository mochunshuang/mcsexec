
#include "../../../include/execution.hpp"
#include "../test_macro.hpp"

namespace test
{
    template <class T, class Env = mcs::execution::empty_env>
    struct expect_error_receiver_ex
    {
        using receiver_concept = mcs::execution::receiver_t;

        explicit expect_error_receiver_ex(T &value) : m_value(&value) {}

        expect_error_receiver_ex(Env env, T &value)
            : m_value(&value), m_env(std::move(env))
        {
        }

        template <class... Ts>
        void set_value(Ts... /*unused*/) noexcept // NOLINT
        {
            UNEXPECT("set_value called on expect_error_receiver_ex");
        }

        void set_stopped() noexcept // NOLINT
        {
            UNEXPECT("set_stopped called on expect_error_receiver_ex");
        }

        template <class Err>
        void set_error(Err /*unused*/) noexcept // NOLINT
        {
            UNEXPECT(
                "set_error called on expect_error_receiver_ex with the wrong error type");
        }

        void set_error(T value) noexcept // NOLINT
        {
            *m_value = std::move(value);
        }

        Env get_env() const noexcept // NOLINT
        {
            return m_env;
        }

      private:
        T *m_value;
        Env m_env{};
    };
}; // namespace test
