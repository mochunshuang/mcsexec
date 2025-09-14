#ifndef C19E32CB_7BD1_49AE_8347_09C1E9E35215
#define C19E32CB_7BD1_49AE_8347_09C1E9E35215

#endif /* C19E32CB_7BD1_49AE_8347_09C1E9E35215 */
#pragma once

#include "../../../include/execution.hpp"
#include "../test_macro.hpp"
#include "./channel.hpp"
#include <any>
#include <utility>

namespace test
{
    struct any_receiver_no_env
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

        constexpr auto get_env() const noexcept // NOLINT
        {

            return mcs::execution::empty_env{};
        }
    };
}; // namespace test