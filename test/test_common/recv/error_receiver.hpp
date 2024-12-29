
#include "../../../include/execution.hpp"
#include "../test_macro.hpp"

namespace test
{
    template <typename T>
    struct error_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;

        bool *called;          // NOLINT
        std::decay_t<T> error; // NOLINT

        template <typename E> // NOLINTNEXTLINE
        auto set_error(E &&e) && noexcept -> void
        {
            *this->called = true;
            EXPECT(error == e);
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return mcs::execution::empty_env{};
        }
    };

    template <typename E>
    error_receiver(bool *, E &&) -> error_receiver<E>;

}; // namespace test
