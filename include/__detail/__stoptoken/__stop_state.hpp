#pragma once

#include <atomic>

#include "./__stop_callback_base.hpp"

namespace mcs::execution::stoptoken
{
    // Note: for impl: type mcs::execution::stoptoken::unspecified
    struct stop_state
    {
        std::atomic<bool> stop_requested;
        std::atomic<::std::size_t> sources;
        std::mutex lock;
        stop_callback_base *callbacks{};
        std::atomic<bool> executing;
    };

}; // namespace mcs::execution::stoptoken