#include <atomic>
#include <cassert>
#include <iostream>

/**
 * @brief 特性	compare_exchange_weak + while	          if + compare_exchange_strong
                伪失败:	可能发生伪失败，需要循环重试	    不会发生伪失败，不需要循环
                性能:	在某些平台上较快，但可能需要重试	在不需要重试的场景中性能更好
                适用场景:	复杂循环或无锁算法	           简单原子操作
 *
 */
class SimpleAtomicOperation
{
  public:
    // 默认构造函数
    SimpleAtomicOperation() : m_atomicVar(0) {}

    // 成员函数模板，接受多个操作
    template <typename... Ops>
    void operator()(Ops &&...ops)
    {
        // 在弱版本会要求循环而强版本不要求时，更偏好强版本
        int expected = 0;
        if (m_atomicVar.compare_exchange_strong(expected, 1))
        {
            (ops(), ...);
            m_atomicVar.store(0);
        }
        // do nothing
    }

  private:
    std::atomic<int> m_atomicVar;
};

int main()
{
    // 创建 AtomicOperation 对象并执行原子操作
    SimpleAtomicOperation atomic_op;
    int a = 0;
    atomic_op([]() { std::cout << "Operation 1" << std::endl; },
              []() { std::cout << "Operation 2" << std::endl; },
              [&]() {
                  std::cout << "Operation 3" << std::endl;
                  a = 2;
              });
    assert(a == 2);
    return 0;
}