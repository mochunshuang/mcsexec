#pragma once

#include <atomic>

namespace mcs::execution::tool
{
    class CasSpinLock
    {

      public:
        void lock() noexcept
        {
            bool expected = false;
            while (!locked.compare_exchange_weak(
                expected, true, std::memory_order_release, std::memory_order_relaxed))
                expected = false;
        }
        void unlock() noexcept
        {
            locked.store(false, std::memory_order_release);
            locked.notify_one();
        }

      private:
        std::atomic<bool> locked = false; // NOLINT
    };

}; // namespace mcs::execution::tool