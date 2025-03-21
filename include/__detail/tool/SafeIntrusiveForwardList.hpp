#pragma once

#include <atomic>
#include <concepts>

namespace mcs::execution::tool
{
    template <typename T>
        requires(requires(T *t) {
            { t->next } -> std::same_as<T *&>;
        })
    class SafeIntrusiveForwardList
    {
      private:
        std::atomic<T *> m_head = nullptr;

      public:
        void pushFront(T *obj)
        {
            T *old_head = m_head.load(std::memory_order_relaxed);
            do // NOLINT
            {
                obj->next = old_head;
            } while (!m_head.compare_exchange_weak(
                old_head, obj, std::memory_order_release, std::memory_order_relaxed));
        }

        void popFront()
        {
            T *old_head = m_head.load(std::memory_order_relaxed);
            while (old_head && !m_head.compare_exchange_weak(old_head, old_head->next,
                                                             std::memory_order_release,
                                                             std::memory_order_relaxed))
                ;
        }

        T *front()
        {
            return m_head.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool empty() const
        {
            return m_head.load(std::memory_order_relaxed) == nullptr;
        }
    };

}; // namespace mcs::execution::tool
