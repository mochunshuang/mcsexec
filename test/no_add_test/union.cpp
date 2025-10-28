#include <iostream>
#include <cassert>

// NOLINTBEGIN
struct TestUnion
{
    union {
        int stack_data[4];
        void *heap_ptr;
    } storage;

    bool use_heap;

    TestUnion(int value) : use_heap(false)
    {
        std::cout << "构造 stack_data: " << value << std::endl;
        storage.stack_data[0] = value;
    }

    TestUnion(void *ptr) : use_heap(true)
    {
        std::cout << "构造 heap_ptr: " << ptr << std::endl;
        storage.heap_ptr = ptr;
    }

    // 默认的拷贝赋值运算符会正确处理 union
    TestUnion &operator=(const TestUnion &other) = default;

    void print() const
    {
        if (use_heap)
        {
            std::cout << "heap_ptr: " << storage.heap_ptr << std::endl;
        }
        else
        {
            std::cout << "stack_data: " << storage.stack_data[0] << std::endl;
        }
    }
};

int main()
{
    // 测试1: 相同类型的赋值
    std::cout << "=== 测试1: 相同类型赋值 ===" << std::endl;
    TestUnion a(100);
    TestUnion b(200);

    std::cout << "赋值前: ";
    a.print();
    b.print();

    assert(a.storage.stack_data[0] == 100);
    assert(b.storage.stack_data[0] == 200);
    assert(a.use_heap == false);
    assert(b.use_heap == false);

    a = b; // 默认赋值操作

    std::cout << "赋值后: ";
    a.print();
    b.print();

    assert(a.storage.stack_data[0] == 200); // a 的值被复制
    assert(b.storage.stack_data[0] == 200); // b 的值不变
    assert(a.use_heap == false);            // use_heap 标志也被复制
    assert(b.use_heap == false);
    std::cout << "✅ 测试1通过：相同类型赋值正确\n" << std::endl;

    // 测试2: 不同类型之间的赋值
    std::cout << "=== 测试2: 不同类型赋值 ===" << std::endl;
    TestUnion c(300);
    int heap_value = 999;
    TestUnion d(&heap_value);

    std::cout << "赋值前: ";
    c.print();
    d.print();

    assert(c.storage.stack_data[0] == 300);
    assert(d.storage.heap_ptr == &heap_value);
    assert(c.use_heap == false);
    assert(d.use_heap == true);

    c = d; // 从 heap_ptr 赋值给 stack_data

    std::cout << "赋值后: ";
    c.print();
    d.print();

    assert(c.storage.heap_ptr == &heap_value); // c 现在指向相同的堆地址
    assert(d.storage.heap_ptr == &heap_value); // d 不变
    assert(c.use_heap == true);                // use_heap 标志被复制
    assert(d.use_heap == true);
    std::cout << "✅ 测试2通过：不同类型赋值正确\n" << std::endl;

    // 测试3: 包含 union 的类赋值
    std::cout << "=== 测试3: 包含 union 的类 ===" << std::endl;
    struct Container
    {
        TestUnion data;
        int extra;

        Container(int val) : data(val), extra(val * 2) {}
        Container(void *ptr) : data(ptr), extra(0) {}

        // 默认赋值操作
        Container &operator=(const Container &) = default;
    };

    Container e(400);
    Container f(500);

    std::cout << "赋值前: e.extra=" << e.extra << ", f.extra=" << f.extra << std::endl;
    e.data.print();
    f.data.print();

    assert(e.data.storage.stack_data[0] == 400);
    assert(f.data.storage.stack_data[0] == 500);
    assert(e.extra == 800);
    assert(f.extra == 1000);

    e = f; // 包含 union 的类的赋值

    std::cout << "赋值后: e.extra=" << e.extra << ", f.extra=" << f.extra << std::endl;
    e.data.print();
    f.data.print();

    assert(e.data.storage.stack_data[0] == 500); // data 被复制
    assert(f.data.storage.stack_data[0] == 500);
    assert(e.extra == 1000); // extra 也被复制
    assert(f.extra == 1000);
    std::cout << "✅ 测试3通过：包含union的类赋值正确\n" << std::endl;

    // 测试4: 自赋值
    std::cout << "=== 测试4: 自赋值 ===" << std::endl;
    TestUnion g(600);
    std::cout << "自赋值前: ";
    g.print();

    assert(g.storage.stack_data[0] == 600);

    g = g; // 自赋值

    std::cout << "自赋值后: ";
    g.print();

    assert(g.storage.stack_data[0] == 600); // 值保持不变
    assert(g.use_heap == false);
    std::cout << "✅ 测试4通过：自赋值正确\n" << std::endl;

    std::cout << "🎉 所有测试通过！C++ 默认赋值操作正确处理包含 union 的类" << std::endl;

    return 0;
}
// NOLINTEND