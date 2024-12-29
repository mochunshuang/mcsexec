#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"

namespace test
{
    enum class Channel : std::uint8_t
    {
        NO_CALL,
        VALUE_CHANNEL,
        ERROR_CHANNEL,
        STOPDE_CHANNEL,
    };

    struct void_receiver
    {
      public:
        bool *called{nullptr};    // NOLINT
        Channel *chanel{nullptr}; // NOLINT
        using receiver_concept = mcs::execution::receiver_t;

        auto set_value() && noexcept -> void // NOLINT
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = Channel::VALUE_CHANNEL;
        }

        template <typename E> // NOLINTNEXTLINE
        auto set_error(E &&e) && noexcept -> void
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = Channel::ERROR_CHANNEL;
        }

        void set_stopped() && noexcept // NOLINT
        {
            *this->called = true;
            if (chanel != nullptr)
                *this->chanel = Channel::STOPDE_CHANNEL;
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return mcs::execution::empty_env{};
        }

        [[nodiscard]] auto status() const
        {
            if (chanel == nullptr)
                return Channel::NO_CALL;
            return *chanel;
        }
    };

}; // namespace test