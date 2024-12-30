#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"
#include "./channel.hpp"
namespace test
{

    struct void_receiver
    {
      public:
        bool *called{nullptr};    // NOLINT
        channel *chanel{nullptr}; // NOLINT
        using receiver_concept = mcs::execution::receiver_t;

        auto set_value() && noexcept -> void // NOLINT
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = channel::VALUE_CHANNEL;
        }

        template <typename E> // NOLINTNEXTLINE
        auto set_error(E &&e) && noexcept -> void
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = channel::ERROR_CHANNEL;
        }

        void set_stopped() && noexcept // NOLINT
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = channel::STOPDE_CHANNEL;
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return mcs::execution::empty_env{};
        }

        [[nodiscard]] auto status() const
        {
            if (chanel == nullptr)
                return channel::NO_CALL;
            return *chanel;
        }
    };

}; // namespace test