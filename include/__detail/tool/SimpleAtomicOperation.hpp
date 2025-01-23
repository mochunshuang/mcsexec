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
        explicit SimpleAtomicOperation(std::atomic_flag &atomicFlag) : locked(atomicFlag)
        {
        }

        // 成员函数模板，接受多个操作
        template <typename... Ops>
        void operator()(Ops &&...ops)
        {
            while (locked.test_and_set(std::memory_order_acquire))
            {
                // C++ 20 起可以仅在 unlock 中通知后才获得锁，从而避免任何无效自旋
                // 注意，即使 wait
                // 保证一定在值被更改后才返回，但锁定是在下一次执行条件时完成的
                locked.wait(true, std::memory_order_relaxed);
            }
            (std::forward<Ops>(ops)(), ...);
            locked.clear(std::memory_order_release); // 释放锁
            locked.notify_one();
        }

      private:
        std::atomic_flag &locked; // NOLINT
    };
}; // namespace mcs::execution::tool