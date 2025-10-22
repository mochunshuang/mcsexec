
#include <iostream>
#include <string>
#include <cassert>

// NOLINTBEGIN

// 统一的构造标签 - 支持默认构造和带参数构造
struct construct_tag
{
    template <typename T, typename... Args>
    void operator()(construct_tag, T *, void *buffer, Args &&...args) const
    {
        new (buffer) T(std::forward<Args>(args)...);
    }
};

struct destruct_tag
{
    template <typename T>
    void operator()(destruct_tag, T *, void *obj) const
    {
        static_cast<T *>(obj)->~T();
    }
};

struct copy_tag
{
    template <typename T>
    void operator()(copy_tag, T *, void *dest, const void *src) const
    {
        new (dest) T(*static_cast<const T *>(src));
    }
};

struct move_tag
{
    template <typename T>
    void operator()(move_tag, T *, void *dest, void *src) const
    {
        new (dest) T(std::move(*static_cast<T *>(src)));
        static_cast<T *>(src)->~T();
    }
};

// 添加拷贝赋值标签
struct copy_assign_tag
{
    template <typename T>
    void operator()(copy_assign_tag, T *, void *dest, const void *src) const
    {
        *static_cast<T *>(dest) = *static_cast<const T *>(src);
    }
};

// 添加移动赋值标签
struct move_assign_tag
{
    template <typename T>
    void operator()(move_assign_tag, T *, void *dest, void *src) const
    {
        *static_cast<T *>(dest) = std::move(*static_cast<T *>(src));
    }
};

struct print_tag
{
    template <typename T>
    void operator()(print_tag, T *, const void *obj) const
    {
        std::cout << *static_cast<const T *>(obj) << std::endl;
    }
};

template <typename T, class... Fns>
struct overload_set : Fns...
{
    using Fns::operator()...;

    template <typename Self, typename Tag, typename... Args>
    void invoke(this Self &&self, Tag tag, Args &&...args)
    {
        std::forward<Self>(self)(tag, static_cast<T *>(nullptr),
                                 std::forward<Args>(args)...);
    }
};

// 测试类 - 跟踪所有操作
class TestClass
{
  public:
    inline static int construct_count = 0;
    inline static int copy_count = 0;
    inline static int move_count = 0;
    inline static int copy_assign_count = 0;
    inline static int move_assign_count = 0;
    inline static int destruct_count = 0;

    int value;
    std::string name;

    // 默认构造
    TestClass() : value(0), name("default")
    {
        ++construct_count;
        std::cout << "默认构造 TestClass(" << value << ", " << name << ")\n";
    }

    // 带参数构造
    TestClass(int v, std::string n = "arg") : value(v), name(std::move(n))
    {
        ++construct_count;
        std::cout << "带参构造 TestClass(" << value << ", " << name << ")\n";
    }

    TestClass(const TestClass &other) : value(other.value), name(other.name + "_copy")
    {
        ++copy_count;
        std::cout << "拷贝构造 TestClass(" << value << ", " << name << ")\n";
    }

    TestClass(TestClass &&other) noexcept
        : value(other.value), name(std::move(other.name) + "_moved")
    {
        other.value = -1;
        ++move_count;
        std::cout << "移动构造 TestClass(" << value << ", " << name << ")\n";
    }

    // 拷贝赋值操作符
    TestClass &operator=(const TestClass &other)
    {
        if (this != &other)
        {
            value = other.value;
            name = other.name + "_copy_assign";
            ++copy_assign_count;
            std::cout << "拷贝赋值 TestClass(" << value << ", " << name << ")\n";
        }
        return *this;
    }

    // 移动赋值操作符
    TestClass &operator=(TestClass &&other) noexcept
    {
        if (this != &other)
        {
            value = other.value;
            name = std::move(other.name) + "_move_assign";
            other.value = -1;
            ++move_assign_count;
            std::cout << "移动赋值 TestClass(" << value << ", " << name << ")\n";
        }
        return *this;
    }

    ~TestClass()
    {
        ++destruct_count;
        std::cout << "析构 TestClass(" << value << ", " << name << ")\n";
    }

    friend std::ostream &operator<<(std::ostream &os, const TestClass &obj)
    {
        return os << "TestClass{" << obj.name << ", " << obj.value << "}";
    }

    static void reset_counts()
    {
        construct_count = copy_count = move_count = copy_assign_count =
            move_assign_count = destruct_count = 0;
    }
};

// 直接对比标准操作和模拟操作
void test_direct_comparison()
{
    std::cout << "=== 直接对比标准操作 vs 模拟操作 ===\n";

    // 测试1: 带参数构造 + 拷贝
    std::cout << "\n--- 测试1: 带参数构造 + 拷贝 ---\n";

    // 标准操作
    std::cout << "标准操作:\n";
    TestClass::reset_counts();
    {
        TestClass a(100); // 带参数构造
        TestClass b = a;  // 拷贝构造
        assert(a.value == 100);
        assert(b.value == 100);
    }
    int std_construct1 = TestClass::construct_count;
    int std_copy1 = TestClass::copy_count;
    int std_destruct1 = TestClass::destruct_count;

    // 模拟操作 - 使用带参数的构造
    std::cout << "\n模拟操作:\n";
    TestClass::reset_counts();
    {
        alignas(TestClass) char buffer_a[sizeof(TestClass)];
        alignas(TestClass) char buffer_b[sizeof(TestClass)];

        overload_set<TestClass, construct_tag, destruct_tag, copy_tag, move_tag> ops;

        ops.invoke(construct_tag{}, buffer_a, 100); // 带参数构造

        ops.invoke(copy_tag{}, buffer_b, buffer_a); // 拷贝构造

        // 验证值
        assert(static_cast<TestClass *>(static_cast<void *>(buffer_a))->value == 100);
        assert(static_cast<TestClass *>(static_cast<void *>(buffer_b))->value == 100);

        ops.invoke(destruct_tag{}, buffer_a); // 析构
        ops.invoke(destruct_tag{}, buffer_b); // 析构
    }
    int sim_construct1 = TestClass::construct_count;
    int sim_copy1 = TestClass::copy_count;
    int sim_destruct1 = TestClass::destruct_count;

    // 验证一致性
    std::cout << "标准: 构造=" << std_construct1 << " 拷贝=" << std_copy1
              << " 析构=" << std_destruct1 << "\n";
    std::cout << "模拟: 构造=" << sim_construct1 << " 拷贝=" << sim_copy1
              << " 析构=" << sim_destruct1 << "\n";

    assert(std_construct1 == sim_construct1);
    assert(std_copy1 == sim_copy1);
    assert(std_destruct1 == sim_destruct1);
    std::cout << "✅ 带参数构造+拷贝测试通过\n";

    // 测试2: 带参数构造 + 移动
    std::cout << "\n--- 测试2: 带参数构造 + 移动 ---\n";

    // 标准操作
    std::cout << "标准操作:\n";
    TestClass::reset_counts();
    {
        TestClass a(200);           // 带参数构造
        TestClass b = std::move(a); // 移动构造
        assert(b.value == 200);
        assert(a.value == -1); // 移动后源对象值被修改
    }
    int std_construct2 = TestClass::construct_count;
    int std_move2 = TestClass::move_count;
    int std_destruct2 = TestClass::destruct_count;

    // 模拟操作
    std::cout << "\n模拟操作:\n";
    TestClass::reset_counts();
    {
        alignas(TestClass) char buffer_a[sizeof(TestClass)];
        alignas(TestClass) char buffer_b[sizeof(TestClass)];

        overload_set<TestClass, construct_tag, destruct_tag, copy_tag, move_tag> ops;

        ops.invoke(construct_tag{}, buffer_a, 200); // 带参数构造

        ops.invoke(move_tag{}, buffer_b, buffer_a); // 移动构造

        // 验证移动语义
        assert(static_cast<TestClass *>(static_cast<void *>(buffer_b))->value == 200);
        // 注意：buffer_a 已经在 move 时被析构，不能再访问

        ops.invoke(destruct_tag{}, buffer_b); // 析构目标对象
    }
    int sim_construct2 = TestClass::construct_count;
    int sim_move2 = TestClass::move_count;
    int sim_destruct2 = TestClass::destruct_count;

    // 验证一致性
    std::cout << "标准: 构造=" << std_construct2 << " 移动=" << std_move2
              << " 析构=" << std_destruct2 << "\n";
    std::cout << "模拟: 构造=" << sim_construct2 << " 移动=" << sim_move2
              << " 析构=" << sim_destruct2 << "\n";

    assert(std_construct2 == sim_construct2);
    assert(std_move2 == sim_move2);
    assert(std_destruct2 == sim_destruct2);
    std::cout << "✅ 带参数构造+移动测试通过\n";
}

// 测试赋值语义
void test_assignment_semantics()
{
    std::cout << "\n=== 测试赋值语义 ===\n";

    overload_set<TestClass, construct_tag, destruct_tag, copy_tag, move_tag,
                 copy_assign_tag, move_assign_tag, print_tag>
        ops;

    // 测试1: 拷贝赋值
    std::cout << "\n--- 测试1: 拷贝赋值 ---\n";
    TestClass::reset_counts();
    {
        alignas(TestClass) char buffer_a[sizeof(TestClass)];
        alignas(TestClass) char buffer_b[sizeof(TestClass)];

        // 构造两个对象
        ops.invoke(construct_tag{}, buffer_a, 300, "source");
        ops.invoke(construct_tag{}, buffer_b, 400, "target");

        std::cout << "赋值前:\n";
        ops.invoke(print_tag{}, buffer_a);
        ops.invoke(print_tag{}, buffer_b);

        // 执行拷贝赋值: b = a
        ops.invoke(copy_assign_tag{}, buffer_b, buffer_a);

        std::cout << "赋值后:\n";
        ops.invoke(print_tag{}, buffer_a);
        ops.invoke(print_tag{}, buffer_b);

        // 验证赋值结果
        auto *obj_a = static_cast<TestClass *>(static_cast<void *>(buffer_a));
        auto *obj_b = static_cast<TestClass *>(static_cast<void *>(buffer_b));
        assert(obj_b->value == 300);
        assert(obj_b->name.find("_copy_assign") != std::string::npos);

        // 验证拷贝赋值计数
        assert(TestClass::copy_assign_count == 1);

        ops.invoke(destruct_tag{}, buffer_a);
        ops.invoke(destruct_tag{}, buffer_b);
    }

    // 测试2: 移动赋值
    std::cout << "\n--- 测试2: 移动赋值 ---\n";
    TestClass::reset_counts();
    {
        alignas(TestClass) char buffer_a[sizeof(TestClass)];
        alignas(TestClass) char buffer_b[sizeof(TestClass)];

        // 构造两个对象
        ops.invoke(construct_tag{}, buffer_a, 500, "source_move");
        ops.invoke(construct_tag{}, buffer_b, 600, "target_move");

        std::cout << "移动赋值前:\n";
        ops.invoke(print_tag{}, buffer_a);
        ops.invoke(print_tag{}, buffer_b);

        // 执行移动赋值: b = std::move(a)
        ops.invoke(move_assign_tag{}, buffer_b, buffer_a);

        std::cout << "移动赋值后:\n";
        ops.invoke(print_tag{}, buffer_b);

        // 验证移动赋值结果
        auto *obj_b = static_cast<TestClass *>(static_cast<void *>(buffer_b));
        assert(obj_b->value == 500);
        assert(obj_b->name.find("_move_assign") != std::string::npos);

        // 验证移动赋值计数
        assert(TestClass::move_assign_count == 1);

        // 注意：移动赋值后源对象可能处于有效但未指定状态
        // 在我们的实现中，我们将其值设为-1
        auto *obj_a = static_cast<TestClass *>(static_cast<void *>(buffer_a));
        assert(obj_a->value == -1);

        ops.invoke(destruct_tag{}, buffer_a);
        ops.invoke(destruct_tag{}, buffer_b);
    }

    // 测试3: 自赋值检查
    std::cout << "\n--- 测试3: 自赋值检查 ---\n";
    TestClass::reset_counts();
    {
        alignas(TestClass) char buffer[sizeof(TestClass)];

        ops.invoke(construct_tag{}, buffer, 700, "self_assign");

        // 自赋值: a = a
        ops.invoke(copy_assign_tag{}, buffer, buffer);

        auto *obj = static_cast<TestClass *>(static_cast<void *>(buffer));
        assert(obj->value == 700);

        ops.invoke(destruct_tag{}, buffer);
    }

    std::cout << "✅ 赋值语义测试通过\n";
}

// 测试统一的构造标签
void test_unified_construct()
{
    std::cout << "=== 测试统一的构造标签 ===\n";

    overload_set<TestClass, construct_tag, destruct_tag, copy_tag, move_tag, print_tag>
        ops;

    // 测试1: 默认构造
    std::cout << "\n--- 测试1: 默认构造 ---\n";
    {
        alignas(TestClass) char buffer[sizeof(TestClass)];
        ops.invoke(construct_tag{}, buffer); // 无参数 -> 默认构造
        assert(static_cast<TestClass *>(static_cast<void *>(buffer))->value == 0);
        assert(static_cast<TestClass *>(static_cast<void *>(buffer))->name == "default");
        ops.invoke(destruct_tag{}, buffer);
    }

    // 测试2: 带一个参数的构造
    std::cout << "\n--- 测试2: 带一个参数的构造 ---\n";
    {
        alignas(TestClass) char buffer[sizeof(TestClass)];
        ops.invoke(construct_tag{}, buffer, 42); // 一个参数
        assert(static_cast<TestClass *>(static_cast<void *>(buffer))->value == 42);
        assert(static_cast<TestClass *>(static_cast<void *>(buffer))->name == "arg");
        ops.invoke(destruct_tag{}, buffer);
    }

    // 测试3: 带两个参数的构造
    std::cout << "\n--- 测试3: 带两个参数的构造 ---\n";
    {
        alignas(TestClass) char buffer[sizeof(TestClass)];
        ops.invoke(construct_tag{}, buffer, 100, "custom"); // 两个参数
        assert(static_cast<TestClass *>(static_cast<void *>(buffer))->value == 100);
        assert(static_cast<TestClass *>(static_cast<void *>(buffer))->name == "custom");
        ops.invoke(destruct_tag{}, buffer);
    }

    std::cout << "✅ 统一构造标签测试通过\n";
}

// 完整功能测试
// 完整功能测试
void test_complete_functionality()
{
    std::cout << "\n=== 完整功能测试 ===\n";

    TestClass::reset_counts();
    overload_set<TestClass, construct_tag, destruct_tag, copy_tag, move_tag,
                 copy_assign_tag, move_assign_tag, print_tag>
        ops;

    {
        alignas(TestClass) char buffer1[sizeof(TestClass)];
        alignas(TestClass) char buffer2[sizeof(TestClass)];
        alignas(TestClass) char buffer3[sizeof(TestClass)];
        alignas(TestClass) char buffer4[sizeof(TestClass)];

        std::cout << "1. 带参数构造\n";
        ops.invoke(construct_tag{}, buffer1, 888, "original");
        ops.invoke(construct_tag{}, buffer4, 999, "assign_target");

        std::cout << "2. 拷贝构造\n";
        ops.invoke(copy_tag{}, buffer2, buffer1);

        std::cout << "3. 移动构造\n";
        // 注意：move_tag 会析构源对象 buffer2
        ops.invoke(move_tag{}, buffer3, buffer2);

        std::cout << "4. 拷贝赋值\n";
        ops.invoke(copy_assign_tag{}, buffer4, buffer1);

        std::cout << "5. 移动赋值\n";
        // 注意：move_assign_tag 不会析构源对象，只是将其置于移动后状态
        ops.invoke(move_assign_tag{}, buffer3, buffer4);

        std::cout << "6. 打印结果\n";
        ops.invoke(print_tag{}, buffer1);
        ops.invoke(print_tag{}, buffer3);

        std::cout << "7. 析构\n";
        // 需要析构的对象：
        // - buffer1: 原始对象
        // - buffer3: 移动赋值后的对象（包含从buffer4移动来的内容）
        // - buffer4: 移动赋值后的源对象（处于移动后状态，但仍需析构）
        // buffer2 已经在移动构造时被 move_tag 析构了
        ops.invoke(destruct_tag{}, buffer1);
        ops.invoke(destruct_tag{}, buffer3);
        ops.invoke(destruct_tag{}, buffer4);
        // 不要再次析构 buffer2，因为它已经在 move_tag 中被析构了
    }

    // 验证计数
    std::cout << "construct_count: " << TestClass::construct_count << '\n';
    std::cout << "copy_count: " << TestClass::copy_count << '\n';
    std::cout << "move_count: " << TestClass::move_count << '\n';
    std::cout << "copy_assign_count: " << TestClass::copy_assign_count << '\n';
    std::cout << "move_assign_count: " << TestClass::move_assign_count << '\n';
    std::cout << "destruct_count: " << TestClass::destruct_count << '\n';

    // 修正断言：总共应该有4次析构
    // 构造：2次 (buffer1, buffer4)
    // 拷贝构造：1次 (buffer2)
    // 移动构造：1次 (buffer3) - 这个会析构buffer2
    // 拷贝赋值：1次 (buffer4 = buffer1)
    // 移动赋值：1次 (buffer3 = buffer4)
    // 手动析构：3次 (buffer1, buffer3, buffer4)
    // 总计析构：移动构造析构1次 + 手动析构3次 = 4次
    assert(TestClass::construct_count == 2);   // 两次构造
    assert(TestClass::copy_count == 1);        // 一次拷贝构造
    assert(TestClass::move_count == 1);        // 一次移动构造
    assert(TestClass::copy_assign_count == 1); // 一次拷贝赋值
    assert(TestClass::move_assign_count == 1); // 一次移动赋值
    assert(TestClass::destruct_count == 4);    // 四次析构

    std::cout << "✅ 完整功能测试通过\n";
}

int main()
{
    test_direct_comparison();
    test_assignment_semantics(); // 新增的赋值语义测试
    test_unified_construct();
    test_complete_functionality();

    std::cout << "\n🎉 所有测试通过！统一的构造标签设计成功\n";
    return 0;
}

// NOLINTEND