#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"
#include "../../sched/MyScheduler.hpp"
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
        struct env_t
        {
            template <class Tag>
                requires(std::is_same_v<Tag, mcs::execution::set_value_t> ||
                         std::is_same_v<Tag, mcs::execution::set_stopped_t>)
            [[nodiscard]] constexpr auto query(
                mcs::execution::queries::get_completion_scheduler_t<Tag> /*unused*/)
                const noexcept
            {
                return MyScheduler();
            }
            [[nodiscard]] constexpr auto query( // NOLINT
                const mcs::execution::queries::get_scheduler_t & /*unused*/)
                const noexcept
            {
                return MyScheduler();
            }
            [[nodiscard]] constexpr auto query(
                const mcs::execution::queries::get_domain_t & /*unused*/) const noexcept
            {
                return mcs::execution::default_domain();
            }
        };
        constexpr auto get_env() const noexcept // NOLINT
        {

            return env_t{};
        }
    };
    template <typename... T>
    value_receiver(bool *, T &&...) -> value_receiver<T...>;
}; // namespace test