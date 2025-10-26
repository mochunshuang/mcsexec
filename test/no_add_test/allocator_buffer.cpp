#include <memory>
#include <iostream>
#include <cassert>
#include <cstring>

// NOLINTBEGIN

// 测试分配器在已有缓冲区上的操作
template <typename T, typename Allocator = std::allocator<std::byte>>
class BufferAllocatorDemo
{
  private:
    static constexpr size_t BUFFER_SIZE = 64;
    alignas(alignof(T)) std::byte buffer_[BUFFER_SIZE];
    Allocator allocator_;
    T *object_ptr_ = nullptr;
    bool using_stack_buffer_ = false;

  public:
    template <typename... Args>
    BufferAllocatorDemo(Args &&...args)
    {
        std::cout << "=== 开始构造对象 ===" << '\n';
        std::cout << "对象大小: " << sizeof(T) << " 字节" << '\n';
        std::cout << "缓冲区大小: " << BUFFER_SIZE << " 字节" << '\n';

        // 重新绑定分配器到目标类型
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc = allocator_; // 修复：使用赋值而不是初始化

        if (sizeof(T) <= BUFFER_SIZE && alignof(T) <= alignof(std::max_align_t))
        {
            // 使用栈缓冲区
            std::cout << "✅ 使用栈缓冲区构造" << '\n';
            object_ptr_ = reinterpret_cast<T *>(buffer_);
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, object_ptr_,
                                                           std::forward<Args>(args)...);
            using_stack_buffer_ = true;
        }
        else
        {
            // 使用堆分配
            std::cout << "🔄 使用堆分配构造" << '\n';
            object_ptr_ = std::allocator_traits<ReboundAlloc>::allocate(rebound_alloc, 1);
            std::cout << "分配地址: " << static_cast<void *>(object_ptr_) << '\n';

            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, object_ptr_,
                                                           std::forward<Args>(args)...);
            using_stack_buffer_ = false;
        }

        std::cout << "对象地址: " << static_cast<void *>(object_ptr_) << '\n';
        std::cout << "缓冲区地址: " << static_cast<void *>(buffer_) << '\n';
    }

    ~BufferAllocatorDemo()
    {
        std::cout << "=== 开始析构对象 ===" << '\n';
        if (object_ptr_)
        {
            using ReboundAlloc =
                typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            ReboundAlloc rebound_alloc = allocator_; // 修复：使用赋值而不是初始化

            std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, object_ptr_);

            if (!using_stack_buffer_)
            {
                std::cout << "释放堆内存: " << static_cast<void *>(object_ptr_) << '\n';
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc,
                                                                object_ptr_, 1);
            }
        }
    }

    T *get()
    {
        return object_ptr_;
    }
    const T *get() const
    {
        return object_ptr_;
    }
    bool is_using_stack_buffer() const
    {
        return using_stack_buffer_;
    }
};

// 小对象 - 适合栈缓冲区
struct SmallObject
{
    int data[8]; // 32字节

    SmallObject(int value)
    {
        for (int i = 0; i < 8; ++i)
            data[i] = value + i;
        std::cout << "SmallObject构造完成" << '\n';
    }

    ~SmallObject()
    {
        std::cout << "SmallObject析构完成" << '\n';
    }

    void print() const
    {
        std::cout << "SmallObject数据: ";
        for (int i = 0; i < 8; ++i)
            std::cout << data[i] << " ";
        std::cout << '\n';
    }
};

// 大对象 - 需要堆分配
struct LargeObject
{
    char data[128]; // 128字节，大于缓冲区

    LargeObject(const char *str)
    {
        std::strncpy(data, str, sizeof(data) - 1); // 修复：去掉std::
        data[sizeof(data) - 1] = '\0';
        std::cout << "LargeObject构造完成" << '\n';
    }

    ~LargeObject()
    {
        std::cout << "LargeObject析构完成" << '\n';
    }

    void print() const
    {
        std::cout << "LargeObject数据: " << data << '\n';
    }
};

// 简化的跟踪分配器
template <typename T>
class TrackingAllocator
{
  public:
    using value_type = T;

    // 添加默认构造函数
    TrackingAllocator() = default;

    // 添加转换构造函数
    template <typename U>
    TrackingAllocator(const TrackingAllocator<U> &)
    {
    }

    T *allocate(size_t n)
    {
        std::cout << "📦 分配 " << n << " 个对象，总大小 " << n * sizeof(T) << " 字节"
                  << '\n';
        return static_cast<T *>(::operator new(n * sizeof(T)));
    }

    void deallocate(T *p, size_t n)
    {
        std::cout << "🗑️  释放 " << n << " 个对象，地址 " << static_cast<void *>(p)
                  << '\n';
        ::operator delete(p);
    }

    template <typename U, typename... Args>
    void construct(U *p, Args &&...args)
    {
        std::cout << "🔨 在地址 " << static_cast<void *>(p) << " 构造对象" << '\n';
        ::new (p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U *p)
    {
        std::cout << "💥 在地址 " << static_cast<void *>(p) << " 析构对象" << '\n';
        p->~U();
    }
};

int main()
{
    std::cout << "=================== 测试1: 小对象 + 默认分配器 ==================="
              << '\n';
    {
        BufferAllocatorDemo<SmallObject> demo1(42);
        demo1.get()->print();
        assert(demo1.is_using_stack_buffer());
        std::cout << "小对象使用栈缓冲区: " << std::boolalpha
                  << demo1.is_using_stack_buffer() << '\n';
    }

    std::cout << "\n=================== 测试2: 大对象 + 默认分配器 ==================="
              << '\n';
    {
        BufferAllocatorDemo<LargeObject> demo2("Hello Large Object");
        demo2.get()->print();
        assert(!demo2.is_using_stack_buffer());
        std::cout << "大对象使用栈缓冲区: " << std::boolalpha
                  << demo2.is_using_stack_buffer() << '\n';
    }

    std::cout << "\n=================== 测试3: 小对象 + 跟踪分配器 ==================="
              << '\n';
    {
        BufferAllocatorDemo<SmallObject, TrackingAllocator<std::byte>> demo3(100);
        demo3.get()->print();
        std::cout << "使用栈缓冲区: " << std::boolalpha << demo3.is_using_stack_buffer()
                  << '\n';
    }

    std::cout << "\n=================== 测试4: 大对象 + 跟踪分配器 ==================="
              << '\n';
    {
        BufferAllocatorDemo<LargeObject, TrackingAllocator<std::byte>> demo4(
            "Tracked Large Object");
        demo4.get()->print();
        std::cout << "使用栈缓冲区: " << std::boolalpha << demo4.is_using_stack_buffer()
                  << '\n';
    }

    return 0;
}
// NOLINTEND