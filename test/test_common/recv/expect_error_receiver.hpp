
#include "./base_expect_receiver.hpp"
#include "./env_tag.hpp"

namespace test
{
    template <class T = std::exception_ptr, class Env = mcs::execution::empty_env>
    struct expect_error_receiver : base_expect_receiver<Env> // NOLINT
    {
        expect_error_receiver() = default;

        explicit expect_error_receiver(T error) : m_error(std::move(error)) {}

        expect_error_receiver(Env env, T error)
            : base_expect_receiver<Env>{std::move(env)}, m_error(std::move(error))
        {
        }

        // these do not move m_error and cannot be defaulted
        expect_error_receiver(expect_error_receiver &&other) noexcept
            : base_expect_receiver<Env>(std::move(other)), m_error()
        {
        }

        expect_error_receiver &operator=(expect_error_receiver &&other) noexcept
        {
            base_expect_receiver<Env>::operator=(std::move(other));
            m_error.reset();
            return *this;
        }

        template <class... Ts>
        void set_value(Ts... /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_value called on expect_error_receiver";
        }

        void set_stopped() noexcept // NOLINT
        {
            boost::ut::expect(false) << "set_stopped called on expect_error_receiver";
        }

        void set_error(T err) noexcept // NOLINT
        {
            this->set_called();
            if (m_error)
            {
                boost::ut::expect(to_comparable(err) == to_comparable(*m_error));
            }
        }

        template <class E>
        void set_error(E /*unused*/) noexcept // NOLINT
        {
            boost::ut::expect(false)
                << "set_error called on expect_error_receiver with wrong error type";
        }

      private:
        std::optional<T> m_error;
    };
}; // namespace test