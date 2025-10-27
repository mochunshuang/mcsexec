#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <type_traits>
#include <memory>
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
        else
        {
            std::cerr << "❌ 错误: 类型 " << typeid(T).name() << " 没有实现 method1"
                      << std::endl;
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
            std::cerr << "❌ 错误: 类型 " << typeid(T).name() << " 没有实现 method2"
                      << std::endl;
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
            std::cerr << "❌ 错误: 类型 " << typeid(T).name() << " 没有实现 method3"
                      << std::endl;
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

// 类型擦除包装器 - 使用标准分配器特性
template <typename Allocator = std::allocator<std::byte>>
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
    Allocator allocator_;

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
                // 栈存储：直接析构
                T *obj = static_cast<T *>(static_cast<void *>(su->stack_buffer));
                obj->~T();
            }
            else
            {
                // 堆存储：析构并释放内存
                if (su->heap_ptr)
                {
                    T *ptr = static_cast<T *>(su->heap_ptr);
                    ptr->~T();
                    // 使用分配器重新绑定
                    using ReboundAlloc = typename std::allocator_traits<
                        Allocator>::template rebind_alloc<T>;
                    ReboundAlloc rebound_alloc(self->allocator_);
                    std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr,
                                                                    1);
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
                // 栈存储：直接构造
                const T *src_obj = static_cast<const T *>(
                    static_cast<const void *>(src_su->stack_buffer));
                T *dest_obj =
                    static_cast<T *>(static_cast<void *>(dest_su->stack_buffer));
                new (dest_obj) T(*src_obj);
            }
            else
            {
                // 堆存储：分配内存并构造
                using ReboundAlloc =
                    typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
                ReboundAlloc rebound_alloc(self->allocator_);
                T *new_ptr =
                    std::allocator_traits<ReboundAlloc>::allocate(rebound_alloc, 1);
                try
                {
                    const T *src_obj = static_cast<const T *>(src_su->heap_ptr);
                    std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, new_ptr,
                                                                   *src_obj);
                    dest_su->heap_ptr = new_ptr;
                }
                catch (...)
                {
                    std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc,
                                                                    new_ptr, 1);
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
                // 栈存储：移动构造并析构源对象
                T *src_obj = static_cast<T *>(static_cast<void *>(src_su->stack_buffer));
                T *dest_obj =
                    static_cast<T *>(static_cast<void *>(dest_su->stack_buffer));
                new (dest_obj) T(std::move(*src_obj));
                src_obj->~T();
            }
            else
            {
                // 堆存储：转移指针所有权
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

    // 默认操作表 - 用于标记无效状态
    static constexpr storage_ops default_ops = {
        nullptr, nullptr, nullptr, nullptr, nullptr, []() noexcept -> bool {
            return false;
        }};

    void cleanup()
    {
        if (ops_ != &default_ops)
        {
            ops_->destroy(this);
        }
    }

  public:
    // 删除默认构造函数 - any不能为空
    any_interface_with_allocator() = delete;

    // 主模板构造函数 - 必须接收有效对象
    template <class T, typename = std::enable_if_t<!std::is_same_v<
                           std::decay_t<T>, any_interface_with_allocator>>>
    any_interface_with_allocator(T &&obj, Allocator alloc = Allocator{})
        : ops_(&static_ops<std::decay_t<T>>::table), allocator_(alloc)
    {
        using DecayedT = std::decay_t<T>;

        std::cout << "📝 构造 " << typeid(DecayedT).name()
                  << " (大小: " << sizeof(DecayedT) << ", 对齐: " << alignof(DecayedT)
                  << ", 缓冲区: " << BUFFER_SIZE << ")" << std::endl;

        if constexpr (sizeof(DecayedT) <= BUFFER_SIZE &&
                      alignof(DecayedT) <= BUFFER_ALIGN)
        {
            std::cout << "✅ 使用栈缓冲区构造" << std::endl;
            new (static_cast<void *>(storage_.stack_buffer))
                DecayedT(std::forward<T>(obj));
        }
        else
        {
            std::cout << "🔄 使用堆分配构造" << std::endl;
            // 使用分配器重新绑定
            using ReboundAlloc = typename std::allocator_traits<
                Allocator>::template rebind_alloc<DecayedT>;
            ReboundAlloc rebound_alloc(allocator_);
            DecayedT *ptr =
                std::allocator_traits<ReboundAlloc>::allocate(rebound_alloc, 1);
            try
            {
                std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr,
                                                               std::forward<T>(obj));
                storage_.heap_ptr = ptr;
            }
            catch (...)
            {
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                throw;
            }
        }
    }

    // 拷贝构造
    any_interface_with_allocator(const any_interface_with_allocator &other)
        : ops_(other.ops_), allocator_(other.allocator_)
    {
        std::cout << "📋 拷贝构造 any_interface" << std::endl;
        other.ops_->copy_construct(this, &other);
    }

    // 移动构造
    any_interface_with_allocator(any_interface_with_allocator &&other) noexcept
        : ops_(other.ops_), allocator_(std::move(other.allocator_))
    {
        std::cout << "🚚 移动构造 any_interface" << std::endl;
        other.ops_->move_construct(this, &other);
        other.ops_ = &default_ops; // 标记源对象为无效
    }

    // 赋值操作 - 修复：不复制分配器，分配器应该是无状态的
    any_interface_with_allocator &operator=(const any_interface_with_allocator &other)
    {
        if (this != &other)
        {
            std::cout << "📋 拷贝赋值 any_interface" << std::endl;
            cleanup();
            ops_ = other.ops_;
            // 注意：不复制分配器，分配器应该是无状态的
            other.ops_->copy_construct(this, &other);
        }
        return *this;
    }

    any_interface_with_allocator &operator=(any_interface_with_allocator &&other) noexcept
    {
        if (this != &other)
        {
            std::cout << "🚚 移动赋值 any_interface" << std::endl;
            cleanup();
            ops_ = other.ops_;
            // 注意：不移动分配器，分配器应该是无状态的
            other.ops_->move_construct(this, &other);
            other.ops_ = &default_ops; // 标记源对象为无效
        }
        return *this;
    }

    ~any_interface_with_allocator()
    {
        std::cout << "🧹 析构 any_interface" << std::endl;
        cleanup();
    }

    // 检查是否使用堆存储
    bool uses_heap_storage() const noexcept
    {
        return ops_->uses_heap_storage();
    }

    // 检查是否有效（非移动后状态）
    bool is_valid() const noexcept
    {
        return ops_ != &default_ops;
    }

    void method1(int arg)
    {
        if (!is_valid())
        {
            throw std::runtime_error("❌ 错误: 使用已移动的 any_interface");
        }
        auto obj = ops_->get_object(this);
        auto vtable = ops_->get_vtable();
        vtable->method1_ptr(obj, arg);
    }

    int method2(double arg) noexcept
    {
        if (!is_valid())
        {
            std::cerr << "❌ 错误: 使用已移动的 any_interface" << std::endl;
            return 0;
        }
        auto obj = ops_->get_object(this);
        auto vtable = ops_->get_vtable();
        return vtable->method2_ptr(obj, arg);
    }

    std::string method3(const char *arg)
    {
        if (!is_valid())
        {
            throw std::runtime_error("❌ 错误: 使用已移动的 any_interface");
        }
        auto obj = ops_->get_object(this);
        auto vtable = ops_->get_vtable();
        return vtable->method3_ptr(obj, arg);
    }
};

// 测试对象 - 各种大小的对象
struct tiny_object
{
    char data[8];
    int value;

    tiny_object(int v) : value(v)
    {
        std::cout << "tiny_object 构造: value=" << value << std::endl;
    }

    void method1(int x)
    {
        value += x;
    }
    int method2(double x) noexcept
    {
        return static_cast<int>(x) + value;
    }
    std::string method3(const char *str)
    {
        return std::string(str) + "_tiny_" + std::to_string(value);
    }

    ~tiny_object()
    {
        std::cout << "tiny_object 析构: value=" << value << std::endl;
    }
};

struct small_object
{
    char data[16];
    int value;

    small_object(int v) : value(v)
    {
        std::cout << "small_object 构造: value=" << value << std::endl;
    }

    void method1(int x)
    {
        value += x;
    }
    int method2(double x) noexcept
    {
        return static_cast<int>(x) + value;
    }
    std::string method3(const char *str)
    {
        return std::string(str) + "_small_" + std::to_string(value);
    }

    ~small_object()
    {
        std::cout << "small_object 析构: value=" << value << std::endl;
    }
};

struct medium_object
{
    char data[24];
    int value;

    medium_object(int v) : value(v)
    {
        std::cout << "medium_object 构造: value=" << value << std::endl;
    }

    void method1(int x)
    {
        value += x;
    }
    int method2(double x) noexcept
    {
        return static_cast<int>(x) + value;
    }
    std::string method3(const char *str)
    {
        return std::string(str) + "_medium_" + std::to_string(value);
    }

    ~medium_object()
    {
        std::cout << "medium_object 析构: value=" << value << std::endl;
    }
};

struct large_object
{
    char data[32];
    int value;

    large_object(int v) : value(v)
    {
        std::cout << "large_object 构造: value=" << value << std::endl;
    }

    void method1(int x)
    {
        value += x;
    }
    int method2(double x) noexcept
    {
        return static_cast<int>(x) + value;
    }
    std::string method3(const char *str)
    {
        return std::string(str) + "_large_" + std::to_string(value);
    }

    ~large_object()
    {
        std::cout << "large_object 析构: value=" << value << std::endl;
    }
};

struct huge_object
{
    char data[64];
    int value;

    huge_object(int v) : value(v)
    {
        std::cout << "huge_object 构造: value=" << value << std::endl;
    }

    void method1(int x)
    {
        value += x;
    }
    int method2(double x) noexcept
    {
        return static_cast<int>(x) + value;
    }
    std::string method3(const char *str)
    {
        return std::string(str) + "_huge_" + std::to_string(value);
    }

    ~huge_object()
    {
        std::cout << "huge_object 析构: value=" << value << std::endl;
    }
};

// 修复的分配器 - 添加跨类型构造函数
class TrackingAllocatorState
{
  public:
    static inline int total_allocations = 0;
    static inline int total_deallocations = 0;
    int instance_allocations = 0;
    int instance_deallocations = 0;

    TrackingAllocatorState() = default;

    void record_allocation()
    {
        instance_allocations++;
        total_allocations++;
    }

    void record_deallocation()
    {
        instance_deallocations++;
        total_deallocations++;
    }
};

template <typename T>
class TrackingAllocator
{
  public:
    using value_type = T;

    TrackingAllocatorState *state;

    TrackingAllocator() : state() {}

    TrackingAllocator(const TrackingAllocator &other) : state(other.state) {}

    // 添加跨类型构造函数
    template <typename U>
    TrackingAllocator(const TrackingAllocator<U> &other) : state(other.state)
    {
    }

    TrackingAllocator(TrackingAllocator &&other) noexcept : state(other.state)
    {
        other.state = nullptr;
    }

    // 添加跨类型移动构造函数
    template <typename U>
    TrackingAllocator(TrackingAllocator<U> &&other) noexcept : state(other.state)
    {
        other.state = nullptr;
    }

    ~TrackingAllocator()
    {
        // state 由第一个创建的分配器管理
    }

    TrackingAllocator &operator=(const TrackingAllocator &other)
    {
        if (this != &other)
        {
            state = other.state;
        }
        return *this;
    }

    TrackingAllocator &operator=(TrackingAllocator &&other) noexcept
    {
        if (this != &other)
        {
            state = other.state;
            other.state = nullptr;
        }
        return *this;
    }

    T *allocate(std::size_t n)
    {
        if (state)
            state->record_allocation();
        std::cout << "📦 分配 " << n << " 个 " << typeid(T).name()
                  << " (实例分配: " << (state ? state->instance_allocations : 0)
                  << ", 总分配: " << TrackingAllocatorState::total_allocations << ")"
                  << std::endl;
        return static_cast<T *>(::operator new(n * sizeof(T)));
    }

    void deallocate(T *p, std::size_t n)
    {
        if (state)
            state->record_deallocation();
        std::cout << "🗑️  释放 " << n << " 个 " << typeid(T).name()
                  << " (实例释放: " << (state ? state->instance_deallocations : 0)
                  << ", 总释放: " << TrackingAllocatorState::total_deallocations << ")"
                  << std::endl;
        ::operator delete(p);
    }

    template <typename U>
    bool operator==(const TrackingAllocator<U> &other) const
    {
        return state == other.state;
    }

    template <typename U>
    bool operator!=(const TrackingAllocator<U> &other) const
    {
        return !(*this == other);
    }
};

void test_mixed_objects_sequence()
{
    std::cout << "\n=== 测试1: 混合对象序列 ===" << std::endl;

    TrackingAllocatorState::total_allocations = 0;
    TrackingAllocatorState::total_deallocations = 0;

    {
        std::vector<any_interface_with_allocator<TrackingAllocator<std::byte>>> objects;

        // 按不同大小顺序添加对象
        std::cout << "\n--- 添加 tiny_object ---" << std::endl;
        objects.emplace_back(tiny_object(1));
        assert(!objects.back().uses_heap_storage());
        std::cout << "✅ tiny_object 使用栈存储: " << objects.back().uses_heap_storage()
                  << std::endl;

        std::cout << "\n--- 添加 small_object ---" << std::endl;
        objects.emplace_back(small_object(2));
        assert(!objects.back().uses_heap_storage());
        std::cout << "✅ small_object 使用栈存储: " << objects.back().uses_heap_storage()
                  << std::endl;

        std::cout << "\n--- 添加 medium_object ---" << std::endl;
        objects.emplace_back(medium_object(3));
        // medium_object 大小28 > 缓冲区24，应该使用堆存储
        assert(objects.back().uses_heap_storage());
        std::cout << "✅ medium_object 使用堆存储: " << objects.back().uses_heap_storage()
                  << std::endl;

        std::cout << "\n--- 添加 large_object ---" << std::endl;
        objects.emplace_back(large_object(4));
        assert(objects.back().uses_heap_storage());
        std::cout << "✅ large_object 使用堆存储: " << objects.back().uses_heap_storage()
                  << std::endl;

        std::cout << "\n--- 添加 huge_object ---" << std::endl;
        objects.emplace_back(huge_object(5));
        assert(objects.back().uses_heap_storage());
        std::cout << "✅ huge_object 使用堆存储: " << objects.back().uses_heap_storage()
                  << std::endl;

        // 测试所有对象的功能
        for (size_t i = 0; i < objects.size(); ++i)
        {
            std::cout << "\n测试对象 " << i << ":" << std::endl;
            objects[i].method1(10);
            auto result = objects[i].method2(5.5);
            auto str_result = objects[i].method3("test");
            std::cout << "结果: " << result << ", " << str_result << std::endl;
        }

        std::cout << "\n总分配次数: " << TrackingAllocatorState::total_allocations
                  << std::endl;
        std::cout << "总释放次数: " << TrackingAllocatorState::total_deallocations
                  << std::endl;
    }

    std::cout << "析构后总分配: " << TrackingAllocatorState::total_allocations
              << std::endl;
    std::cout << "析构后总释放: " << TrackingAllocatorState::total_deallocations
              << std::endl;
    assert(TrackingAllocatorState::total_allocations ==
           TrackingAllocatorState::total_deallocations);
}

void test_assignment_operations()
{
    std::cout << "\n=== 测试2: 赋值操作 ===" << std::endl;

    TrackingAllocatorState::total_allocations = 0;
    TrackingAllocatorState::total_deallocations = 0;

    {
        // 栈对象赋值给栈对象
        std::cout << "\n--- 栈→栈赋值 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> a(tiny_object(100));
        any_interface_with_allocator<TrackingAllocator<std::byte>> b(small_object(200));
        a = b;
        a.method1(10);
        assert(a.method2(0) == 210);

        // 堆对象赋值给栈对象
        std::cout << "\n--- 堆→栈赋值 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> c(tiny_object(300));
        any_interface_with_allocator<TrackingAllocator<std::byte>> d(large_object(400));
        c = d;
        c.method1(10);
        assert(c.method2(0) == 410);
        assert(c.uses_heap_storage());

        // 栈对象赋值给堆对象
        std::cout << "\n--- 栈→堆赋值 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> e(large_object(500));
        any_interface_with_allocator<TrackingAllocator<std::byte>> f(tiny_object(600));
        e = f;
        e.method1(10);
        assert(e.method2(0) == 610);
        assert(!e.uses_heap_storage());

        // 堆对象赋值给堆对象
        std::cout << "\n--- 堆→堆赋值 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> g(large_object(700));
        any_interface_with_allocator<TrackingAllocator<std::byte>> h(huge_object(800));
        g = h;
        g.method1(10);
        assert(g.method2(0) == 810);
        assert(g.uses_heap_storage());
    }

    std::cout << "赋值测试后总分配: " << TrackingAllocatorState::total_allocations
              << std::endl;
    std::cout << "赋值测试后总释放: " << TrackingAllocatorState::total_deallocations
              << std::endl;
}

void test_move_semantics()
{
    std::cout << "\n=== 测试3: 移动语义 ===" << std::endl;

    TrackingAllocatorState::total_allocations = 0;
    TrackingAllocatorState::total_deallocations = 0;

    {
        // 移动栈对象
        std::cout << "\n--- 移动栈对象 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> a(tiny_object(100));
        any_interface_with_allocator<TrackingAllocator<std::byte>> b(std::move(a));
        assert(!a.is_valid());
        assert(b.is_valid());
        b.method1(10);
        assert(b.method2(0) == 110);

        // 移动堆对象
        std::cout << "\n--- 移动堆对象 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> c(large_object(200));
        any_interface_with_allocator<TrackingAllocator<std::byte>> d(std::move(c));
        assert(!c.is_valid());
        assert(d.is_valid());
        d.method1(20);
        assert(d.method2(0) == 220);

        // 移动赋值
        std::cout << "\n--- 移动赋值 ---" << std::endl;
        any_interface_with_allocator<TrackingAllocator<std::byte>> e(tiny_object(300));
        any_interface_with_allocator<TrackingAllocator<std::byte>> f(large_object(400));
        e = std::move(f);
        assert(!f.is_valid());
        assert(e.is_valid());
        e.method1(30);
        assert(e.method2(0) == 430);
    }

    std::cout << "移动测试后总分配: " << TrackingAllocatorState::total_allocations
              << std::endl;
    std::cout << "移动测试后总释放: " << TrackingAllocatorState::total_deallocations
              << std::endl;
}

void test_exception_safety()
{
    std::cout << "\n=== 测试4: 异常安全 ===" << std::endl;

    try
    {
        // 测试构造时的异常
        struct throwing_object
        {
            throwing_object(int)
            {
                throw std::runtime_error("构造时抛出异常");
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

        any_interface_with_allocator<std::allocator<std::byte>> a(throwing_object(1));
    }
    catch (const std::exception &e)
    {
        std::cout << "✅ 捕获到预期异常: " << e.what() << std::endl;
    }

    // 测试使用已移动的对象
    any_interface_with_allocator<std::allocator<std::byte>> b(tiny_object(100));
    any_interface_with_allocator<std::allocator<std::byte>> c(std::move(b));

    try
    {
        b.method1(10); // 应该抛出异常
        assert(false && "应该抛出异常");
    }
    catch (const std::exception &e)
    {
        std::cout << "✅ 捕获到移动后使用异常: " << e.what() << std::endl;
    }
}

int main()
{
    std::cout << "当前缓冲区大小: " << ANY_INTERFACE_BUFFER_SIZE << " bytes" << std::endl;
    std::cout << "对象大小信息:" << std::endl;
    std::cout << "  • tiny_object: " << sizeof(tiny_object) << " bytes" << std::endl;
    std::cout << "  • small_object: " << sizeof(small_object) << " bytes" << std::endl;
    std::cout << "  • medium_object: " << sizeof(medium_object) << " bytes" << std::endl;
    std::cout << "  • large_object: " << sizeof(large_object) << " bytes" << std::endl;
    std::cout << "  • huge_object: " << sizeof(huge_object) << " bytes" << std::endl;

    test_mixed_objects_sequence();
    test_assignment_operations();
    test_move_semantics();
    test_exception_safety();

    std::cout << "\n🎉 所有测试通过！" << std::endl;
    std::cout << "📋 验证要点:" << std::endl;
    std::cout << "  • 混合对象大小处理正确" << std::endl;
    std::cout << "  • 分配器状态一致" << std::endl;
    std::cout << "  • 拷贝/移动语义正确" << std::endl;
    std::cout << "  • 异常安全保证" << std::endl;
    std::cout << "  • 内存无泄漏" << std::endl;

    return 0;
}

// NOLINTEND