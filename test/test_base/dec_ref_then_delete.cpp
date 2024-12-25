#include <cassert>
#include <iostream>
#include <atomic> // 包含 std::atomic

class MyClass
{
  public:
    MyClass() : ref_count(1)
    {
        std::cout << "MyClass()\n";
    }

    void inc_ref() noexcept // NOLINT
    {
        ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    void dec_ref() noexcept // NOLINT
    {
        if (ref_count.fetch_sub(1, std::memory_order_release) == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
            std::cout << "***********MyClass() 真正被销毁***********\n";
        }
    }

    void doSomething() // NOLINT
    {
        std::cout << "ref_count:" << ref_count.load() << "\n";
    }

    std::atomic<int> ref_count; // NOLINT
};

int main()
{
    auto *obj1 = new MyClass();

    {
        // Note: 指针：共享同一块内存
        MyClass *obj2 = obj1;
        obj2->inc_ref(); // 引用计数增加到 2
        std::cout << "obj2 使用共享对象\n";
        obj2->doSomething();
        assert(obj2->ref_count.load() == 2);

    }; // obj2 超出作用域

    std::cout << "注意: obj2 超出作用域，但引用计数没有减少\n";

    obj1->dec_ref(); // 引用计数减少到 1，没有被销毁
    assert(obj1->ref_count.load() == 1);
    std::cout << "注意: 再次dec_ref后才 delete \n";
    obj1->dec_ref();
    return 0;
}