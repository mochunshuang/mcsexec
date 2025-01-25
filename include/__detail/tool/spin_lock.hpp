#pragma once

#include <atomic>

namespace mcs::execution::tool
{
    /**
     * @brief 性能不如自定义 自旋的循环
     *
     */
    struct spin_lock
    {
        mutable std::atomic_flag lock_; // NOLINT

        void lock() const noexcept
        {
            while (lock_.test_and_set(std::memory_order_acquire))
                // C++ 20 起可以仅在 unlock 中通知后才获得锁，从而避免任何无效自旋
                // 注意，即使 wait
                // 保证一定在值被更改后才返回，但锁定是在下一次执行条件时完成的
                lock_.wait(true, std::memory_order_relaxed);
        }
        bool try_lock() const noexcept // NOLINT
        {
            return !lock_.test_and_set(std::memory_order_acquire);
        }
        void unlock() const noexcept
        {
            lock_.clear(std::memory_order_release);
            lock_.notify_one();
        }
    };
}; // namespace mcs::execution::tool
