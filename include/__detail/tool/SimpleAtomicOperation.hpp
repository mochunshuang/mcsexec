#pragma once
#include <atomic>
#include <utility>

namespace mcs::execution::tool
{
    /**
     * @brief 特性	compare_exchange_weak + while	          if + compare_exchange_strong
                    伪失败:	可能发生伪失败，需要循环重试	    不会发生伪失败，不需要循环
                    性能:	在某些平台上较快，但可能需要重试
     在不需要重试的场景中性能更好 适用场景:	复杂循环或无锁算法	           简单原子操作
     *
     */
    class SimpleAtomicOperation
    {
      public:
        // 默认构造函数
        explicit SimpleAtomicOperation(std::atomic<bool> &atomicVar) : locked(atomicVar)
        {
        }

        // 成员函数模板，接受多个操作
        template <typename... Ops>
        void operator()(Ops &&...ops)
        {
            bool expected = false;
            while (!locked.compare_exchange_weak(
                expected, true, std::memory_order_acquire, std::memory_order_relaxed))
            {
                // 如果 expected 被修改为其他值，则重置为 false
                expected = false;
            }
            (std::forward<Ops>(ops)(), ...);
            locked.store(false, std::memory_order_release);
        }

      private:
        std::atomic<bool> &locked; // NOLINT
    };
}; // namespace mcs::execution::tool