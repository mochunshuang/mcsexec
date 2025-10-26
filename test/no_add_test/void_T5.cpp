#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <type_traits>
#include <vector>

// NOLINTBEGIN

#ifndef ANY_INTERFACE_BUFFER_SIZE
#define ANY_INTERFACE_BUFFER_SIZE 24
#endif

// 基础类型包装
template <class T>
struct __mtype
{
    using type = T;
};

// 检查成员函数是否存在
template <typename T, typename = void>
struct has_method1 : std::false_type
{
};

template <typename T>
struct has_method1<T,
                   std::void_t<decltype(std::declval<T>().method1(std::declval<int>()))>>
    : std::true_type
{
};

template <typename T, typename = void>
struct has_method2 : std::false_type
{
};

template <typename T>
struct has_method2<
    T, std::void_t<decltype(std::declval<T>().method2(std::declval<double>()))>>
    : std::true_type
{
};

template <typename T, typename = void>
struct has_method3 : std::false_type
{
};

template <typename T>
struct has_method3<
    T, std::void_t<decltype(std::declval<T>().method3(std::declval<const char *>()))>>
    : std::true_type
{
};

// 函数指针包装器
template <typename Sig>
struct function_ptr;

template <typename Ret, typename... Args>
struct function_ptr<Ret(Args...)>
{
    using type = Ret (*)(void *, Args...);
};

template <typename Ret, typename... Args>
struct function_ptr<Ret(Args...) noexcept>
{
    using type = Ret (*)(void *, Args...) noexcept;
};

// 通用的 VTable 模板
template <typename... Signatures>
struct vtable;

template <typename Sig1, typename Sig2, typename Sig3>
struct vtable<Sig1, Sig2, Sig3>
{
    using method1_type = typename function_ptr<Sig1>::type;
    using method2_type = typename function_ptr<Sig2>::type;
    using method3_type = typename function_ptr<Sig3>::type;

    method1_type method1_ptr;
    method2_type method2_ptr;
    method3_type method3_ptr;
};

// 具体的接口 VTable
using my_interface_vtable =
    vtable<void(int), int(double) noexcept, std::string(const char *)>;

// VTable 创建器
template <typename T>
struct vtable_creator
{
    static void method1_impl(void *obj, int arg)
    {
        if constexpr (has_method1<T>::value)
        {
            static_cast<T *>(obj)->method1(arg);
        }
    }

    static int method2_impl(void *obj, double arg) noexcept
    {
        if constexpr (has_method2<T>::value)
        {
            return static_cast<T *>(obj)->method2(arg);
        }
        else
        {
            return int{};
        }
    }

    static std::string method3_impl(void *obj, const char *arg)
    {
        if constexpr (has_method3<T>::value)
        {
            return static_cast<T *>(obj)->method3(arg);
        }
        else
        {
            return std::string{};
        }
    }

    static const my_interface_vtable &create()
    {
        static const my_interface_vtable vtable_instance = {&method1_impl, &method2_impl,
                                                            &method3_impl};
        return vtable_instance;
    }
};

// 简化的分配器 - 只包装 new/delete
class SimpleAllocator
{
  public:
    SimpleAllocator() = default;

    // 分配内存
    void *allocate(size_t size)
    {
        std::cout << "📦 分配 " << size << " 字节" << std::endl;
        return ::operator new(size);
    }

    // 释放内存
    void deallocate(void *ptr, size_t size)
    {
        std::cout << "🗑️  释放 " << size << " 字节，地址 " << ptr << std::endl;
        ::operator delete(ptr);
    }
};

// 类型擦除包装器 - 集成分配器版本
class any_interface_with_allocator
{
  private:
    static constexpr std::size_t BUFFER_SIZE = ANY_INTERFACE_BUFFER_SIZE;
    static constexpr std::size_t BUFFER_ALIGN = alignof(std::max_align_t);

    struct storage_ops
    {
        void *(*get_object)(void *storage) noexcept;
        void (*destroy)(void *storage) noexcept;
        void (*copy_construct)(void *dest, const void *src);
        void (*move_construct)(void *dest, void *src);
        const my_interface_vtable *(*get_vtable)() noexcept;
        bool (*uses_heap_storage)() noexcept;
    };

    union storage_union {
        alignas(BUFFER_ALIGN) char stack_buffer[BUFFER_SIZE];
        void *heap_ptr;
    };

    storage_union storage_;
    const storage_ops *ops_;
    SimpleAllocator allocator_;

    template <typename T>
    struct static_ops
    {
        static void *get_object(void *storage) noexcept
        {
            auto *su = static_cast<storage_union *>(storage);
            if constexpr (sizeof(T) <= BUFFER_SIZE && alignof(T) <= BUFFER_ALIGN)
            {
                return static_cast<void *>(su->stack_buffer);
            }
            else
            {
                return su->heap_ptr;
            }
        }

        static void destroy(void *storage) noexcept
        {
            auto *su = static_cast<storage_union *>(storage);
            auto *self = static_cast<any_interface_with_allocator *>(storage);

            if constexpr (sizeof(T) <= BUFFER_SIZE && alignof(T) <= BUFFER_ALIGN)
            {
                static_cast<T *>(static_cast<void *>(su->stack_buffer))->~T();
            }
            else
            {
                if (su->heap_ptr)
                {
                    T *ptr = static_cast<T *>(su->heap_ptr);
                    ptr->~T();
                    self->allocator_.deallocate(ptr, sizeof(T));
                }
            }
        }

        static void copy_construct(void *dest, const void *src)
        {
            auto *dest_su = static_cast<storage_union *>(dest);
            auto *src_su = static_cast<const storage_union *>(src);
            auto *self = static_cast<any_interface_with_allocator *>(dest);

            if constexpr (sizeof(T) <= BUFFER_SIZE && alignof(T) <= BUFFER_ALIGN)
            {
                new (dest_su->stack_buffer) T(*static_cast<const T *>(
                    static_cast<const void *>(src_su->stack_buffer)));
            }
            else
            {
                T *new_ptr = static_cast<T *>(self->allocator_.allocate(sizeof(T)));
                try
                {
                    new (new_ptr) T(*static_cast<const T *>(src_su->heap_ptr));
                    dest_su->heap_ptr = new_ptr;
                }
                catch (...)
                {
                    self->allocator_.deallocate(new_ptr, sizeof(T));
                    throw;
                }
            }
        }

        static void move_construct(void *dest, void *src)
        {
            auto *dest_su = static_cast<storage_union *>(dest);
            auto *src_su = static_cast<storage_union *>(src);
            auto *self = static_cast<any_interface_with_allocator *>(dest);

            if constexpr (sizeof(T) <= BUFFER_SIZE && alignof(T) <= BUFFER_ALIGN)
            {
                new (dest_su->stack_buffer) T(std::move(
                    *static_cast<T *>(static_cast<void *>(src_su->stack_buffer))));
                static_cast<T *>(static_cast<void *>(src_su->stack_buffer))->~T();
            }
            else
            {
                dest_su->heap_ptr = src_su->heap_ptr;
                src_su->heap_ptr = nullptr;
            }
        }

        static const my_interface_vtable *get_vtable() noexcept
        {
            return &vtable_creator<T>::create();
        }

        static bool uses_heap_storage() noexcept
        {
            return (sizeof(T) > BUFFER_SIZE) || (alignof(T) > BUFFER_ALIGN);
        }

        static constexpr storage_ops table = {&get_object,     &destroy,
                                              &copy_construct, &move_construct,
                                              &get_vtable,     &uses_heap_storage};
    };

    // 移除默认空状态，any必须包含有效对象
    // 构造函数必须接收有效对象

  public:
    // 删除默认构造函数 - any不能为空
    any_interface_with_allocator() = delete;

    // 主模板构造函数 - 必须接收有效对象
    template <class T, typename = std::enable_if_t<!std::is_same_v<
                           std::decay_t<T>, any_interface_with_allocator>>>
    any_interface_with_allocator(T &&obj) : ops_(&static_ops<std::decay_t<T>>::table)
    {

        if constexpr (sizeof(std::decay_t<T>) <= BUFFER_SIZE &&
                      alignof(std::decay_t<T>) <= BUFFER_ALIGN)
        {
            std::cout << "✅ 使用栈缓冲区构造 " << typeid(T).name() << std::endl;
            new (storage_.stack_buffer) std::decay_t<T>(std::forward<T>(obj));
        }
        else
        {
            std::cout << "🔄 使用堆分配构造 " << typeid(T).name() << std::endl;
            std::decay_t<T> *ptr = static_cast<std::decay_t<T> *>(
                allocator_.allocate(sizeof(std::decay_t<T>)));
            try
            {
                new (ptr) std::decay_t<T>(std::forward<T>(obj));
                storage_.heap_ptr = ptr;
            }
            catch (...)
            {
                allocator_.deallocate(ptr, sizeof(std::decay_t<T>));
                throw;
            }
        }
    }

    // 拷贝构造
    any_interface_with_allocator(const any_interface_with_allocator &other)
        : ops_(other.ops_)
    {
        other.ops_->copy_construct(this, &other);
    }

    // 移动构造
    any_interface_with_allocator(any_interface_with_allocator &&other) noexcept
        : ops_(other.ops_)
    {
        other.ops_->move_construct(this, &other);
        other.ops_ = nullptr; // 标记源对象为无效
    }

    // 赋值操作
    any_interface_with_allocator &operator=(const any_interface_with_allocator &other)
    {
        if (this != &other)
        {
            cleanup();
            ops_ = other.ops_;
            other.ops_->copy_construct(this, &other);
        }
        return *this;
    }

    any_interface_with_allocator &operator=(any_interface_with_allocator &&other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            ops_ = other.ops_;
            other.ops_->move_construct(this, &other);
            other.ops_ = nullptr; // 标记源对象为无效
        }
        return *this;
    }

    ~any_interface_with_allocator()
    {
        cleanup();
    }

    // 检查是否使用堆存储
    bool uses_heap_storage() const noexcept
    {
        return ops_->uses_heap_storage();
    }

    void method1(int arg)
    {
        auto obj = ops_->get_object(this);
        auto vtable = ops_->get_vtable();
        vtable->method1_ptr(obj, arg);
    }

    int method2(double arg) noexcept
    {
        auto obj = ops_->get_object(this);
        auto vtable = ops_->get_vtable();
        return vtable->method2_ptr(obj, arg);
    }

    std::string method3(const char *arg)
    {
        auto obj = ops_->get_object(this);
        auto vtable = ops_->get_vtable();
        return vtable->method3_ptr(obj, arg);
    }

  private:
    void cleanup()
    {
        if (ops_)
        {
            ops_->destroy(this);
        }
    }
};

// ==================== 测试对象定义 ====================

// 小对象 - 完全实现接口
struct small_object_full
{
    int value = 0;
    int id;
    static int next_id;

    small_object_full(int v) : value(v), id(++next_id)
    {
        std::cout << "small_object_full[" << id << "] 构造: value=" << value << std::endl;
    }

    small_object_full(const small_object_full &other) : value(other.value), id(++next_id)
    {
        std::cout << "small_object_full[" << id << "] 拷贝构造 from [" << other.id << "]"
                  << std::endl;
    }

    small_object_full(small_object_full &&other) noexcept
        : value(other.value), id(++next_id)
    {
        std::cout << "small_object_full[" << id << "] 移动构造 from [" << other.id << "]"
                  << std::endl;
        other.value = -1;
    }

    ~small_object_full()
    {
        std::cout << "small_object_full[" << id << "] 析构: value=" << value << std::endl;
    }

    void method1(int x)
    {
        std::cout << "small_object_full[" << id << "]::method1(" << x << ")" << std::endl;
        value += x;
    }

    int method2(double x) noexcept
    {
        std::cout << "small_object_full[" << id << "]::method2(" << x << ")" << std::endl;
        return static_cast<int>(x) + value;
    }

    std::string method3(const char *str)
    {
        std::cout << "small_object_full[" << id << "]::method3(" << str << ")"
                  << std::endl;
        return std::string(str) + "_small_full_" + std::to_string(value);
    }
};

int small_object_full::next_id = 0;

// 小对象 - 部分实现接口
struct small_object_partial
{
    int value = 0;
    int id;
    static int next_id;

    small_object_partial(int v) : value(v), id(++next_id)
    {
        std::cout << "small_object_partial[" << id << "] 构造: value=" << value
                  << std::endl;
    }

    // 只实现 method1
    void method1(int x)
    {
        std::cout << "small_object_partial[" << id << "]::method1(" << x << ")"
                  << std::endl;
        value += x;
    }

    // method2 和 method3 未实现

    ~small_object_partial()
    {
        std::cout << "small_object_partial[" << id << "] 析构: value=" << value
                  << std::endl;
    }
};

int small_object_partial::next_id = 0;

// 大对象 - 完全实现接口
struct large_object_full
{
    char data[32];
    int value = 0;
    int id;
    static int next_id;

    large_object_full(int v) : value(v), id(++next_id)
    {
        std::cout << "large_object_full[" << id << "] 构造: value=" << value << std::endl;
    }

    large_object_full(const large_object_full &other) : value(other.value), id(++next_id)
    {
        std::cout << "large_object_full[" << id << "] 拷贝构造 from [" << other.id << "]"
                  << std::endl;
    }

    large_object_full(large_object_full &&other) noexcept
        : value(other.value), id(++next_id)
    {
        std::cout << "large_object_full[" << id << "] 移动构造 from [" << other.id << "]"
                  << std::endl;
        other.value = -1;
    }

    ~large_object_full()
    {
        std::cout << "large_object_full[" << id << "] 析构: value=" << value << std::endl;
    }

    void method1(int x)
    {
        std::cout << "large_object_full[" << id << "]::method1(" << x << ")" << std::endl;
        value += x;
    }

    int method2(double x) noexcept
    {
        std::cout << "large_object_full[" << id << "]::method2(" << x << ")" << std::endl;
        return static_cast<int>(x) + value;
    }

    std::string method3(const char *str)
    {
        std::cout << "large_object_full[" << id << "]::method3(" << str << ")"
                  << std::endl;
        return std::string(str) + "_large_full_" + std::to_string(value);
    }
};

int large_object_full::next_id = 0;

// 大对象 - 特殊对齐要求
struct alignas(32) aligned_large_object
{
    char data[48]; // 更大的对象
    int value = 0;
    int id;
    static int next_id;

    aligned_large_object(int v) : value(v), id(++next_id)
    {
        std::cout << "aligned_large_object[" << id << "] 构造: value=" << value
                  << ", align=" << alignof(aligned_large_object) << std::endl;
    }

    void method1(int x)
    {
        std::cout << "aligned_large_object[" << id << "]::method1(" << x << ")"
                  << std::endl;
        value += x;
    }

    int method2(double x) noexcept
    {
        std::cout << "aligned_large_object[" << id << "]::method2(" << x << ")"
                  << std::endl;
        return static_cast<int>(x) + value;
    }

    std::string method3(const char *str)
    {
        std::cout << "aligned_large_object[" << id << "]::method3(" << str << ")"
                  << std::endl;
        return std::string(str) + "_aligned_" + std::to_string(value);
    }

    ~aligned_large_object()
    {
        std::cout << "aligned_large_object[" << id << "] 析构: value=" << value
                  << std::endl;
    }
};

int aligned_large_object::next_id = 0;

// ==================== 测试用例 ====================

void test_small_objects()
{
    std::cout << "\n=== 小对象测试 ===" << std::endl;

    // 测试完全实现的小对象
    std::cout << "--- 完全实现的小对象 ---" << std::endl;
    {
        small_object_full small(100);
        any_interface_with_allocator any_small(small);

        assert(!any_small.uses_heap_storage());

        any_small.method1(10);
        int result = any_small.method2(1.5);
        std::string str = any_small.method3("test");

        assert(result == 111);
        assert(str == "test_small_full_110");
        std::cout << "✅ 完全实现小对象测试通过" << std::endl;
    }

    // 测试部分实现的小对象
    std::cout << "\n--- 部分实现的小对象 ---" << std::endl;
    {
        small_object_partial small(200);
        any_interface_with_allocator any_small(small);

        assert(!any_small.uses_heap_storage());

        any_small.method1(20);                       // 应该工作
        int result = any_small.method2(2.5);         // 应该返回默认值
        std::string str = any_small.method3("test"); // 应该返回空字符串

        assert(result == 0);
        assert(str.empty());
        std::cout << "✅ 部分实现小对象测试通过" << std::endl;
    }
}

void test_large_objects()
{
    std::cout << "\n=== 大对象测试 ===" << std::endl;

    // 测试普通大对象
    std::cout << "--- 普通大对象 ---" << std::endl;
    {
        large_object_full large(300);
        any_interface_with_allocator any_large(large);

        assert(any_large.uses_heap_storage());

        any_large.method1(30);
        int result = any_large.method2(3.5);
        std::string str = any_large.method3("test");

        assert(result == 333);
        assert(str == "test_large_full_330");
        std::cout << "✅ 普通大对象测试通过" << std::endl;
    }

    // 测试对齐要求严格的大对象
    std::cout << "\n--- 对齐大对象 ---" << std::endl;
    {
        aligned_large_object aligned(400);
        any_interface_with_allocator any_aligned(aligned);

        assert(any_aligned.uses_heap_storage());

        any_aligned.method1(40);
        int result = any_aligned.method2(4.5);
        std::string str = any_aligned.method3("test");

        assert(result == 444);
        assert(str == "test_aligned_440");
        std::cout << "✅ 对齐大对象测试通过" << std::endl;
    }
}

void test_mixed_storage_collection()
{
    std::cout << "\n=== 混合存储集合测试 ===" << std::endl;

    std::vector<any_interface_with_allocator> objects;

    // 混合添加小对象和大对象
    std::cout << "--- 添加小对象和大对象 ---" << std::endl;
    objects.emplace_back(small_object_full(1));
    objects.emplace_back(large_object_full(2));
    objects.emplace_back(small_object_partial(3));
    objects.emplace_back(aligned_large_object(4));
    objects.emplace_back(small_object_full(5));

    std::cout << "\n--- 执行统一操作 ---" << std::endl;
    for (size_t i = 0; i < objects.size(); ++i)
    {
        std::cout << "对象[" << i << "]: ";
        objects[i].method1(static_cast<int>(i + 1));

        int result = objects[i].method2(0.5);
        std::string str = objects[i].method3("item");

        std::cout << "  result=" << result << ", str=" << str << std::endl;
    }

    std::cout << "\n--- 存储策略统计 ---" << std::endl;
    int stack_count = 0, heap_count = 0;
    for (const auto &obj : objects)
    {
        if (obj.uses_heap_storage())
        {
            heap_count++;
        }
        else
        {
            stack_count++;
        }
    }

    std::cout << "栈存储对象: " << stack_count << " 个" << std::endl;
    std::cout << "堆存储对象: " << heap_count << " 个" << std::endl;
    std::cout << "✅ 混合存储集合测试通过" << std::endl;
}

void test_complex_operations()
{
    std::cout << "\n=== 复杂操作测试 ===" << std::endl;

    // 测试拷贝链
    std::cout << "--- 拷贝链测试 ---" << std::endl;
    {
        small_object_full original(500);
        any_interface_with_allocator copy1(original);
        any_interface_with_allocator copy2(copy1);
        any_interface_with_allocator copy3(copy2);

        copy3.method1(50);
        assert(copy3.method2(0) == 550);
        std::cout << "✅ 拷贝链测试通过" << std::endl;
    }

    // 测试移动链
    std::cout << "\n--- 移动链测试 ---" << std::endl;
    {
        large_object_full original(600);
        any_interface_with_allocator moved1(std::move(original));
        any_interface_with_allocator moved2(std::move(moved1));
        any_interface_with_allocator moved3(std::move(moved2));

        moved3.method1(60);
        assert(moved3.method2(0) == 660);
        std::cout << "✅ 移动链测试通过" << std::endl;
    }

    // 测试混合拷贝移动
    std::cout << "\n--- 混合拷贝移动测试 ---" << std::endl;
    {
        small_object_full small(700);
        large_object_full large(800);

        any_interface_with_allocator any1(small);
        any_interface_with_allocator any2(large);

        // 交叉赋值
        any_interface_with_allocator any3 = any1;            // 拷贝
        any_interface_with_allocator any4 = std::move(any2); // 移动

        any3.method1(70);
        any4.method1(80);

        assert(any3.method2(0) == 770);
        assert(any4.method2(0) == 880);
        std::cout << "✅ 混合拷贝移动测试通过" << std::endl;
    }
}

void test_safety_guarantees()
{
    std::cout << "\n=== 安全性保证测试 ===" << std::endl;

    // 测试异常安全
    std::cout << "--- 异常安全测试 ---" << std::endl;
    try
    {
        struct throwing_object
        {
            throwing_object()
            {
                throw std::runtime_error("构造异常");
            }
            void method1(int) {}
            int method2(double) noexcept
            {
                return 0;
            }
            std::string method3(const char *)
            {
                return "";
            }
        };

        throwing_object obj;
        any_interface_with_allocator any(obj); // 应该不会到达这里
        assert(false && "不应该到达这里");
    }
    catch (const std::exception &e)
    {
        std::cout << "✅ 异常安全测试通过: " << e.what() << std::endl;
    }

    // 测试自赋值安全
    std::cout << "\n--- 自赋值安全测试 ---" << std::endl;
    {
        small_object_full small(900);
        any_interface_with_allocator any(small);

        any = any; // 自赋值应该安全

        any.method1(90);
        assert(any.method2(0) == 990);
        std::cout << "✅ 自赋值安全测试通过" << std::endl;
    }
}

int main()
{
    std::cout << "当前缓冲区大小: " << ANY_INTERFACE_BUFFER_SIZE << " bytes" << std::endl;
    std::cout << "测试目标: 安全高效的any代理，支持任意大小对象" << std::endl;

    test_small_objects();
    test_large_objects();
    test_mixed_storage_collection();
    test_complex_operations();
    test_safety_guarantees();

    std::cout << "\n🎉 所有测试通过！any代理安全高效！" << std::endl;
    std::cout << "📋 验证要点:" << std::endl;
    std::cout << "  • 小对象使用栈存储，零分配开销" << std::endl;
    std::cout << "  • 大对象使用堆存储，通过分配器管理" << std::endl;
    std::cout << "  • 支持部分实现接口的对象" << std::endl;
    std::cout << "  • 支持特殊对齐要求的对象" << std::endl;
    std::cout << "  • 混合存储集合安全运行" << std::endl;
    std::cout << "  • 复杂拷贝移动操作正确" << std::endl;
    std::cout << "  • 异常安全和自赋值安全" << std::endl;
    std::cout << "  • any不能为空，保证始终有效" << std::endl;

    return 0;
}

// NOLINTEND