#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"
#include "./channel.hpp"
#include "../../sched/MyScheduler.hpp"
#include <any>
#include <utility>

namespace test
{
    struct any_receiver
    {
        using receiver_concept = mcs::execution::receiver_t;
        bool *called;    // NOLINT
        std::any *data;  // NOLINT
        channel *chanel; // NOLINT

        template <typename... A> // NOLINTNEXTLINE
        auto set_value(A &&...a) && noexcept -> void
        {
            *this->called = true;
            if constexpr (sizeof...(A) > 0)
            {
                *this->data = std::make_tuple(std::forward<A>(a)...);
            }
            if (chanel != nullptr)
                *this->chanel = channel::VALUE_CHANNEL;
        }

        template <typename E> // NOLINTNEXTLINE
        auto set_error(E &&e) && noexcept -> void
        {
            *this->called = true;
            *this->data = std::forward<E>(e);
            if (chanel != nullptr)
                *this->chanel = channel::ERROR_CHANNEL;
        }

        void set_stopped() && noexcept // NOLINT
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = channel::STOPDE_CHANNEL;
        }

        auto &refData()
        {
            return data;
        }

        [[nodiscard]] auto status() const
        {
            if (chanel == nullptr)
                return channel::NO_CALL;
            return *chanel;
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
}; // namespace test