#include <iostream>
#include <cassert>

// NOLINTBEGIN
// 没有构造函数的类，包含 union
struct NoConstructor
{
    union {
        int stack_data[4];
        void *heap_ptr;
    } storage;

    bool use_heap;
    // 没有构造函数！
};

int main()
{
    std::cout << "=== 测试没有构造函数的类 ===" << std::endl;

    // 测试1: 默认初始化 + 手动赋值
    std::cout << "测试1: 默认初始化 + 手动赋值" << std::endl;
    NoConstructor a;
    NoConstructor b;

    // 手动初始化
    a.storage.stack_data[0] = 100;
    a.use_heap = false;

    b.storage.heap_ptr = (void *)0x1234;
    b.use_heap = true;

    std::cout << "赋值前: a.stack_data[0]=" << a.storage.stack_data[0]
              << ", b.heap_ptr=" << b.storage.heap_ptr << std::endl;

    a = b; // 默认赋值操作

    std::cout << "赋值后: a.heap_ptr=" << a.storage.heap_ptr
              << ", b.heap_ptr=" << b.storage.heap_ptr << std::endl;

    assert(a.storage.heap_ptr == (void *)0x1234);
    assert(b.storage.heap_ptr == (void *)0x1234);
    assert(a.use_heap == true);
    assert(b.use_heap == true);
    std::cout << "✅ 测试1通过\n" << std::endl;

    // 测试2: 栈数据到堆指针的转换
    std::cout << "测试2: 栈数据到堆指针的转换" << std::endl;
    NoConstructor c;
    NoConstructor d;

    c.storage.stack_data[0] = 200;
    c.use_heap = false;

    d.storage.heap_ptr = (void *)0x5678;
    d.use_heap = true;

    std::cout << "赋值前: c.stack_data[0]=" << c.storage.stack_data[0]
              << ", d.heap_ptr=" << d.storage.heap_ptr << std::endl;

    c = d; // 赋值操作

    std::cout << "赋值后: c.heap_ptr=" << c.storage.heap_ptr
              << ", d.heap_ptr=" << d.storage.heap_ptr << std::endl;

    assert(c.storage.heap_ptr == (void *)0x5678);
    assert(c.use_heap == true);
    std::cout << "✅ 测试2通过\n" << std::endl;

    // 测试3: 自赋值
    std::cout << "测试3: 自赋值" << std::endl;
    NoConstructor e;
    e.storage.stack_data[0] = 300;
    e.use_heap = false;

    std::cout << "自赋值前: e.stack_data[0]=" << e.storage.stack_data[0] << std::endl;

    e = e; // 自赋值

    std::cout << "自赋值后: e.stack_data[0]=" << e.storage.stack_data[0] << std::endl;

    assert(e.storage.stack_data[0] == 300);
    assert(e.use_heap == false);
    std::cout << "✅ 测试3通过\n" << std::endl;

    // 测试4: 包含 union 的复杂结构
    std::cout << "测试4: 复杂结构赋值" << std::endl;
    struct ComplexStruct
    {
        NoConstructor data;
        int extra_info;
        // 没有构造函数！
    };

    ComplexStruct f, g;
    f.data.storage.stack_data[0] = 400;
    f.data.use_heap = false;
    f.extra_info = 800;

    g.data.storage.heap_ptr = (void *)0x9ABC;
    g.data.use_heap = true;
    g.extra_info = 1600;

    std::cout << "赋值前: f.extra_info=" << f.extra_info
              << ", g.extra_info=" << g.extra_info << std::endl;

    f = g; // 默认赋值

    std::cout << "赋值后: f.extra_info=" << f.extra_info
              << ", g.extra_info=" << g.extra_info << std::endl;

    assert(f.data.storage.heap_ptr == (void *)0x9ABC);
    assert(f.data.use_heap == true);
    assert(f.extra_info == 1600);
    std::cout << "✅ 测试4通过\n" << std::endl;

    std::cout << "🎉 所有测试通过！没有构造函数的类也能安全使用默认赋值操作" << std::endl;

    return 0;
}
// NOLINTEND