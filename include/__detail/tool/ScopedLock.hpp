#pragma once

namespace mcs::execution::tool
{
    template <typename Lock>
    class ScopedLock
    {
      public:
        explicit ScopedLock(Lock &lock) noexcept : m_lock(lock)
        {
            m_lock.lock();
        }

        ~ScopedLock() noexcept
        {
            m_lock.unlock();
        }

        ScopedLock(const ScopedLock &) = delete;
        ScopedLock &operator=(const ScopedLock &) = delete;
        ScopedLock(ScopedLock &&) = delete;
        ScopedLock &operator=(ScopedLock &&) = delete;

      private:
        Lock &m_lock;
    };

}; // namespace mcs::execution::tool