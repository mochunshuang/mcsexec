#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <type_traits>
#include <array>

// NOLINTBEGIN

// 允许通过宏注入 BUFFER_SIZE
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

// 虚函数包装器
template <class Sig>
struct __vfun;

template <class Ret, class... Args>
struct __vfun<Ret(Args...)>
{
    Ret (*__fn_)(void *, Args...);

    Ret operator()(void *obj, Args... args) const
    {
        return __fn_(obj, std::forward<Args>(args)...);
    }
};

template <class Ret, class... Args>
struct __vfun<Ret(Args...) noexcept>
{
    Ret (*__fn_)(void *, Args...) noexcept;

    Ret operator()(void *obj, Args... args) const noexcept
    {
        return __fn_(obj, std::forward<Args>(args)...);
    }
};

// VTable定义
struct my_interface_vtable
{
    using signatures =
        std::tuple<void(int), int(double) noexcept, std::string(const char *)>;
    static constexpr std::size_t size = std::tuple_size_v<signatures>;
    std::array<const void *, size> __vfuncs{};

    // 访问器方法
    auto method1() const -> __vfun<void(int)>
    {
        return *static_cast<const __vfun<void(int)> *>(__vfuncs[0]);
    }

    auto method2() const -> __vfun<int(double) noexcept>
    {
        return *static_cast<const __vfun<int(double) noexcept> *>(__vfuncs[1]);
    }

    auto method3() const -> __vfun<std::string(const char *)>
    {
        return *static_cast<const __vfun<std::string(const char *)> *>(__vfuncs[2]);
    }
};

// VTable创建器
struct __create_vtable_t
{
    template <class VTable, class T>
    constexpr auto operator()(__mtype<VTable>, __mtype<T>) const noexcept
        -> const VTable *
    {
        return __create_impl<VTable, T>();
    }

  private:
    template <class VTable, class T>
    static constexpr auto __create_impl() noexcept -> const VTable *
    {
        static const VTable vtable = __make_vtable<VTable, T>();
        return &vtable;
    }

    template <class VTable, class T>
    static constexpr auto __make_vtable() noexcept -> VTable
    {
        VTable vtable{};
        __fill_vtable<0, VTable, T>(vtable);
        return vtable;
    }

    template <std::size_t I, class VTable, class T>
    static constexpr void __fill_vtable(VTable &vtable)
    {
        if constexpr (I < VTable::size)
        {
            using Sig = std::tuple_element_t<I, typename VTable::signatures>;
            vtable.__vfuncs[I] = __make_vfun_for<Sig, T>();
            __fill_vtable<I + 1, VTable, T>(vtable);
        }
    }

    template <class Sig, class T>
    static constexpr auto __make_vfun_for() noexcept -> const void *
    {
        static const __vfun<Sig> vfun = __create_vfun<Sig, T>();
        return &vfun;
    }

    template <class Sig, class T>
    static constexpr auto __create_vfun() noexcept -> __vfun<Sig>
    {
        if constexpr (std::is_same_v<Sig, void(int)>)
        {
            if constexpr (has_method1<T>::value)
            {
                return __vfun<Sig>{+[](void *obj, int arg) -> void {
                    static_cast<T *>(obj)->method1(arg);
                }};
            }
            else
            {
                return __vfun<Sig>{+[](void *obj, int) -> void {
                }};
            }
        }
        else if constexpr (std::is_same_v<Sig, int(double) noexcept>)
        {
            if constexpr (has_method2<T>::value)
            {
                return __vfun<Sig>{+[](void *obj, double arg) noexcept -> int {
                    return static_cast<T *>(obj)->method2(arg);
                }};
            }
            else
            {
                return __vfun<Sig>{+[](void *obj, double) noexcept -> int {
                    return int{};
                }};
            }
        }
        else if constexpr (std::is_same_v<Sig, std::string(const char *)>)
        {
            if constexpr (has_method3<T>::value)
            {
                return __vfun<Sig>{+[](void *obj, const char *arg) -> std::string {
                    return static_cast<T *>(obj)->method3(arg);
                }};
            }
            else
            {
                return __vfun<Sig>{+[](void *obj, const char *) -> std::string {
                    return std::string{};
                }};
            }
        }
        else
        {
            static_assert(sizeof(Sig) == 0, "Unsupported signature");
        }
    }
};

inline constexpr __create_vtable_t __create_vtable{};

// 标签系统
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

struct copy_assign_tag
{
    template <typename T>
    void operator()(copy_assign_tag, T *, void *dest, const void *src) const
    {
        *static_cast<T *>(dest) = *static_cast<const T *>(src);
    }
};

struct move_assign_tag
{
    template <typename T>
    void operator()(move_assign_tag, T *, void *dest, void *src) const
    {
        *static_cast<T *>(dest) = std::move(*static_cast<T *>(src));
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

// 类型擦除包装器 - 纯函数指针实现，支持大小对象
class any_interface
{
  private:
    static constexpr std::size_t BUFFER_SIZE = ANY_INTERFACE_BUFFER_SIZE;
    static constexpr std::size_t BUFFER_ALIGN = alignof(std::max_align_t);

    // 存储操作函数表
    struct storage_ops
    {
        void *(*get_object)(void *storage) noexcept;
        void (*destroy)(void *storage) noexcept;
        void (*copy_construct)(void *dest, const void *src);
        void (*move_construct)(void *dest, void *src);
    };

    // 存储联合：小对象用内联缓冲区，大对象用堆指针
    union storage_union {
        alignas(BUFFER_ALIGN) char stack_buffer[BUFFER_SIZE];
        void *heap_ptr;
    };

    storage_union storage_;
    const storage_ops *ops_;
    const my_interface_vtable *vtable_;
    bool uses_heap_;

    // 为特定类型生成操作表
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
            if constexpr (sizeof(T) <= BUFFER_SIZE && alignof(T) <= BUFFER_ALIGN)
            {
                static_cast<T *>(static_cast<void *>(su->stack_buffer))->~T();
            }
            else
            {
                delete static_cast<T *>(su->heap_ptr);
            }
        }

        static void copy_construct(void *dest, const void *src)
        {
            auto *dest_su = static_cast<storage_union *>(dest);
            auto *src_su = static_cast<const storage_union *>(src);

            if constexpr (sizeof(T) <= BUFFER_SIZE && alignof(T) <= BUFFER_ALIGN)
            {
                new (dest_su->stack_buffer) T(*static_cast<const T *>(
                    static_cast<const void *>(src_su->stack_buffer)));
            }
            else
            {
                dest_su->heap_ptr = new T(*static_cast<const T *>(src_su->heap_ptr));
            }
        }

        static void move_construct(void *dest, void *src)
        {
            auto *dest_su = static_cast<storage_union *>(dest);
            auto *src_su = static_cast<storage_union *>(src);

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

        static constexpr storage_ops table = {&get_object, &destroy, &copy_construct,
                                              &move_construct};

        static constexpr bool uses_heap =
            (sizeof(T) > BUFFER_SIZE) || (alignof(T) > BUFFER_ALIGN);
    };

    template <typename T>
    static const my_interface_vtable *get_vtable() noexcept
    {
        return __create_vtable(__mtype<my_interface_vtable>{}, __mtype<T>{});
    }

    static constexpr storage_ops default_ops = {nullptr, nullptr, nullptr, nullptr};

    void cleanup()
    {
        if (ops_ != &default_ops)
        {
            ops_->destroy(&storage_);
        }
    }

  public:
    // 默认构造
    any_interface() noexcept : ops_(&default_ops), vtable_(nullptr), uses_heap_(false)
    {
        storage_.heap_ptr = nullptr;
    }

    // 主模板构造函数
    template <class T, typename = std::enable_if_t<
                           !std::is_same_v<std::decay_t<T>, any_interface>>>
    any_interface(T &&obj)
        : ops_(&static_ops<std::decay_t<T>>::table),
          vtable_(get_vtable<std::decay_t<T>>()),
          uses_heap_(static_ops<std::decay_t<T>>::uses_heap)
    {
        if constexpr (sizeof(std::decay_t<T>) <= BUFFER_SIZE &&
                      alignof(std::decay_t<T>) <= BUFFER_ALIGN)
        {
            new (storage_.stack_buffer) std::decay_t<T>(std::forward<T>(obj));
        }
        else
        {
            storage_.heap_ptr = new std::decay_t<T>(std::forward<T>(obj));
        }
    }

    // 拷贝构造
    any_interface(const any_interface &other)
        : ops_(other.ops_), vtable_(other.vtable_), uses_heap_(other.uses_heap_)
    {
        if (other.ops_ != &default_ops)
        {
            other.ops_->copy_construct(&storage_, &other.storage_);
        }
        else
        {
            storage_.heap_ptr = nullptr;
        }
    }

    // 移动构造
    any_interface(any_interface &&other) noexcept
        : ops_(other.ops_), vtable_(other.vtable_), uses_heap_(other.uses_heap_)
    {
        if (other.ops_ != &default_ops)
        {
            other.ops_->move_construct(&storage_, &other.storage_);
            other.ops_ = &default_ops;
            other.vtable_ = nullptr;
            other.uses_heap_ = false;
        }
        else
        {
            storage_.heap_ptr = nullptr;
        }
    }

    // 赋值操作
    any_interface &operator=(const any_interface &other)
    {
        if (this != &other)
        {
            cleanup();
            ops_ = other.ops_;
            vtable_ = other.vtable_;
            uses_heap_ = other.uses_heap_;
            if (other.ops_ != &default_ops)
            {
                other.ops_->copy_construct(&storage_, &other.storage_);
            }
        }
        return *this;
    }

    any_interface &operator=(any_interface &&other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            ops_ = other.ops_;
            vtable_ = other.vtable_;
            uses_heap_ = other.uses_heap_;
            if (other.ops_ != &default_ops)
            {
                other.ops_->move_construct(&storage_, &other.storage_);
            }
            other.ops_ = &default_ops;
            other.vtable_ = nullptr;
            other.uses_heap_ = false;
        }
        return *this;
    }

    ~any_interface()
    {
        cleanup();
    }

    // 检查是否为空
    bool empty() const noexcept
    {
        return ops_ == &default_ops;
    }

    // 检查是否使用堆存储
    bool uses_heap_storage() const noexcept
    {
        return uses_heap_;
    }

    void method1(int arg)
    {
        if (!empty())
        {
            auto obj = ops_->get_object(&storage_);
            vtable_->method1()(obj, arg);
        }
    }

    int method2(double arg) noexcept
    {
        if (!empty())
        {
            auto obj = ops_->get_object(&storage_);
            return vtable_->method2()(obj, arg);
        }
        return int{};
    }

    std::string method3(const char *arg)
    {
        if (!empty())
        {
            auto obj = ops_->get_object(&storage_);
            return vtable_->method3()(obj, arg);
        }
        return std::string{};
    }
};

// 测试大对象 - 调整为超过24字节
struct large_object
{
    char data[32]; // 32字节，超过24字节的缓冲区
    int value = 0;

    large_object() = default;
    large_object(int v) : value(v)
    {
        std::cout << "large_object 构造: value=" << value << ", size=" << sizeof(*this)
                  << " bytes\n";
    }

    void method1(int x)
    {
        std::cout << "large_object::method1(" << x << "), value=" << value << "\n";
        value += x;
    }

    int method2(double x) noexcept
    {
        std::cout << "large_object::method2(" << x << "), value=" << value << "\n";
        return static_cast<int>(x) + value;
    }

    std::string method3(const char *str)
    {
        std::cout << "large_object::method3(" << str << "), value=" << value << "\n";
        return std::string(str) + "_large_" + std::to_string(value);
    }
};

// 测试小对象 - 调整为小于24字节
struct small_object
{
    int value = 0;

    small_object() = default;
    small_object(int v) : value(v)
    {
        std::cout << "small_object 构造: value=" << value << ", size=" << sizeof(*this)
                  << " bytes\n";
    }

    void method1(int x)
    {
        std::cout << "small_object::method1(" << x << "), value=" << value << "\n";
        value += x;
    }

    int method2(double x) noexcept
    {
        std::cout << "small_object::method2(" << x << "), value=" << value << "\n";
        return static_cast<int>(x) + value;
    }

    std::string method3(const char *str)
    {
        std::cout << "small_object::method3(" << str << "), value=" << value << "\n";
        return std::string(str) + "_small_" + std::to_string(value);
    }
};

void test_edge_cases()
{
    std::cout << "\n=== 测试边界情况 ===\n";

    // 测试1: 空对象操作
    std::cout << "--- 测试空对象 ---\n";
    any_interface empty_obj;
    empty_obj.method1(100);                            // 应该无操作
    int empty_result = empty_obj.method2(1.0);         // 应该返回默认值
    std::string empty_str = empty_obj.method3("test"); // 应该返回空字符串
    assert(empty_result == 0);
    assert(empty_str.empty());
    std::cout << "空对象测试通过\n";

    // 测试2: 自赋值
    std::cout << "--- 测试自赋值 ---\n";
    small_object self_test(50);
    any_interface self_obj(self_test);
    self_obj = self_obj; // 自赋值应该安全
    self_obj.method1(1);
    std::cout << "自赋值测试通过\n";

    // 测试3: 移动后源对象状态
    std::cout << "--- 测试移动语义 ---\n";
    small_object move_test(60);
    any_interface source_obj(move_test);
    any_interface moved_obj = std::move(source_obj);

    // 移动后源对象应该为空
    assert(source_obj.empty());
    assert(!moved_obj.empty());
    moved_obj.method1(1);
    std::cout << "移动语义测试通过\n";

    // 测试4: 异常安全 - 测试部分实现的对象
    struct partially_implemented
    {
        void method1(int x)
        {
            std::cout << "partial::method1(" << x << ")\n";
        }
        // 缺少 method2 和 method3
    };

    partially_implemented partial;
    any_interface any_partial(partial);
    any_partial.method1(99);                                // 应该工作
    int partial_result = any_partial.method2(3.14);         // 应该返回默认值
    std::string partial_str = any_partial.method3("hello"); // 应该返回空字符串
    assert(partial_result == 0);
    assert(partial_str.empty());
    std::cout << "部分实现对象测试通过\n";

    // 测试5: 对齐要求严格的对象
    struct alignas(32) aligned_object
    {
        char data[32];
        int value = 0;

        void method1(int x)
        {
            std::cout << "aligned_object::method1(" << x << ")\n";
            value += x;
        }
        int method2(double x) noexcept
        {
            return static_cast<int>(x) + value;
        }
        std::string method3(const char *str)
        {
            return std::string(str) + "_aligned";
        }
    };

    aligned_object aligned;
    any_interface any_aligned(aligned);
    any_aligned.method1(10);
    std::cout << "对齐对象测试通过\n";
}

void test_performance_characteristics()
{
    std::cout << "\n=== 测试性能特性 ===\n";

    // 测试存储类型决策是否正确
    struct exactly_buffer_size
    {
        char data[ANY_INTERFACE_BUFFER_SIZE]; // 正好等于缓冲区大小
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

    struct slightly_larger
    {
        char data[ANY_INTERFACE_BUFFER_SIZE + 1]; // 比缓冲区大1字节
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

    exactly_buffer_size exact;
    slightly_larger larger;

    any_interface any_exact(exact);
    any_interface any_larger(larger);

    std::cout << "正好缓冲区大小对象(" << sizeof(exactly_buffer_size)
              << " bytes): " << (any_exact.uses_heap_storage() ? "堆存储" : "栈存储")
              << "\n";
    std::cout << "稍大对象(" << sizeof(slightly_larger)
              << " bytes): " << (any_larger.uses_heap_storage() ? "堆存储" : "栈存储")
              << "\n";

    assert(!any_exact.uses_heap_storage()); // 应该使用栈存储
    assert(any_larger.uses_heap_storage()); // 应该使用堆存储
    std::cout << "存储决策测试通过\n";
}

static int construct_count;
static int destruct_count;

void test_lifetime_management_simple()
{
    std::cout << "\n=== 测试生命周期管理（简化版） ===\n";

    struct tracked_object
    {
        int value;

        tracked_object(int v = 0) : value(v)
        {
            ++construct_count;
            std::cout << "tracked_object 构造 #" << construct_count << "\n";
        }

        tracked_object(const tracked_object &other) : value(other.value)
        {
            ++construct_count;
            std::cout << "tracked_object 拷贝构造 #" << construct_count << "\n";
        }

        ~tracked_object()
        {
            ++destruct_count;
            std::cout << "tracked_object 析构 #" << destruct_count << "\n";
        }

        void method1(int x)
        {
            value += x;
        }
        int method2(double) noexcept
        {
            return value;
        }
        std::string method3(const char *str)
        {
            return std::string(str);
        }
    };

    // 测试1: 简单构造和析构
    std::cout << "--- 测试1: 简单构造析构 ---\n";
    construct_count = 0;
    destruct_count = 0;
    {
        tracked_object obj(100);    // 构造 #1
        any_interface any_obj(obj); // 拷贝构造 #2
        any_obj.method1(10);
    }
    assert(construct_count == 2 && destruct_count == 2);
    std::cout << "✅ 简单构造析构测试通过\n";

    // 测试2: 拷贝构造
    std::cout << "--- 测试2: 拷贝构造 ---\n";
    construct_count = 0;
    destruct_count = 0;
    {
        tracked_object obj(200);   // 构造 #1
        any_interface any1(obj);   // 拷贝构造 #2
        any_interface any2 = any1; // 拷贝构造 #3
        any2.method1(20);
    }
    assert(construct_count == 3 && destruct_count == 3);
    std::cout << "✅ 拷贝构造测试通过\n";

    // 测试3: 移动构造
    std::cout << "--- 测试3: 移动构造 ---\n";
    construct_count = 0;
    destruct_count = 0;
    {
        tracked_object obj(300);              // 构造 #1
        any_interface any1(obj);              // 拷贝构造 #2
        any_interface any2 = std::move(any1); // 移动构造 #3
        any2.method1(30);
    }
    assert(construct_count == 3 && destruct_count == 3);
    std::cout << "✅ 移动构造测试通过\n";

    std::cout << "生命周期管理测试全部通过\n";
}

int main()
{
    std::cout << "当前缓冲区大小: " << ANY_INTERFACE_BUFFER_SIZE << " bytes\n";

    std::cout << "\n=== 测试小对象（栈存储）===\n";
    small_object small(100);
    any_interface any_small(small);

    any_small.method1(10);
    int result_small = any_small.method2(1.5);
    std::string str_small = any_small.method3("small_test");

    std::cout << "小对象结果: " << result_small << ", " << str_small << "\n";
    std::cout << "小对象存储类型: "
              << (any_small.uses_heap_storage() ? "堆存储" : "栈存储") << "\n";

    // 测试小对象拷贝
    any_interface any_small_copy = any_small;
    any_small_copy.method1(20);

    std::cout << "\n=== 测试大对象（堆存储）===\n";
    large_object large(200);
    any_interface any_large(large);

    any_large.method1(30);
    int result_large = any_large.method2(2.5);
    std::string str_large = any_large.method3("large_test");

    std::cout << "大对象结果: " << result_large << ", " << str_large << "\n";
    std::cout << "大对象存储类型: "
              << (any_large.uses_heap_storage() ? "堆存储" : "栈存储") << "\n";

    // 测试大对象拷贝
    any_interface any_large_copy = any_large;
    any_large_copy.method1(40);

    // 测试移动
    std::cout << "\n=== 测试大对象移动 ===\n";
    any_interface any_large_moved = std::move(any_large_copy);
    any_large_moved.method1(50);

    std::cout << "✅ 所有测试通过！大小对象都支持！\n";

    test_edge_cases();
    test_performance_characteristics();
    test_lifetime_management_simple();

    return 0;
}

// NOLINTEND