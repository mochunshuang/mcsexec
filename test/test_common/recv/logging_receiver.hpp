
#include "../../../include/execution.hpp"

namespace test
{
    struct logging_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;

        explicit logging_receiver(int &state) : m_state(&state) {}

        template <class... Args>
        void set_value(Args...) noexcept // NOLINT
        {
            *m_state = 0;
        }

        void set_stopped() noexcept // NOLINT
        {
            *m_state = 1;
        }

        template <class E>
        void set_error(E) noexcept // NOLINT
        {
            *m_state = 2;
        }

        mcs::execution::empty_env get_env() const noexcept // NOLINT
        {
            return {};
        }

      private:
        int *m_state;
    };

}; // namespace test
