#pragma once

#include <concepts>

namespace mcs::execution::tool
{
    template <typename T>
        requires(requires(T *t) {
            { t->next } -> std::same_as<T *&>;
        })
    class IntrusiveForwardList
    {
      private:
        T *m_head = nullptr;

      public:
        void pushFront(T *obj)
        {
            obj->next = m_head;
            m_head = obj;
        }

        void popFront()
        {
            if (m_head)
            {
                m_head = m_head->next;
            }
        }

        T *front()
        {
            return m_head;
        }

        [[nodiscard]] bool empty() const
        {
            return m_head == nullptr;
        }
    };

}; // namespace mcs::execution::tool