#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"

namespace test
{
    template <typename... T>
    struct value_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;
        bool *called;                          // NOLINT
        std::tuple<std::decay_t<T>...> expect; // NOLINT

        // NOLINTNEXTLINE
        value_receiver(bool *called_ptr, T &&...values)
            : called(called_ptr), expect(std::forward<T>(values)...)
        {
        }

        template <typename... A> // NOLINTNEXTLINE
        auto set_value(A &&...a) && noexcept -> void
        {
            *this->called = true;
            if constexpr (sizeof...(T) > 0)
            {
                [this, &a...]<std::size_t... I>(std::index_sequence<I...>) {
                    EXPECT(((std::get<I>(expect) == a) && ...));
                }(std::index_sequence_for<T...>{});
            }
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return mcs::execution::empty_env{};
        }
    };
    template <typename... T>
    value_receiver(bool *, T &&...) -> value_receiver<T...>;
}; // namespace test