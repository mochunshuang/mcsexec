#pragma once

#include <cstdint>

namespace test
{
    enum class channel : std::uint8_t
    {
        NO_CALL,
        VALUE_CHANNEL,
        ERROR_CHANNEL,
        STOPDE_CHANNEL,
    };
}; // namespace test