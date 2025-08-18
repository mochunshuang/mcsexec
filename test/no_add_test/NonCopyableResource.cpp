#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// NOLINTBEGIN

template <typename T, typename Allocator = std::allocator<T>>
class AsyncResourceManager
{
    using Traits = std::allocator_traits<Allocator>;
    using AllocTraits = std::allocator_traits<typename Traits::template rebind_alloc<T>>;

    typename Traits::pointer ptr_;
    Allocator alloc_;
    bool released_ = false;

  public:
    explicit AsyncResourceManager(const Allocator &alloc = Allocator())
        : alloc_(alloc), ptr_(nullptr)
    {
    }

    ~AsyncResourceManager()
    {
        if (!released_ && ptr_)
        {
            destroy();
        }
    }

    // 禁止拷贝和移动
    AsyncResourceManager(const AsyncResourceManager &) = delete;
    AsyncResourceManager &operator=(const AsyncResourceManager &) = delete;
    AsyncResourceManager(AsyncResourceManager &&) = delete;
    AsyncResourceManager &operator=(AsyncResourceManager &&) = delete;

    // 分配内存并构造对象
    template <typename... Args>
    T *create(Args &&...args)
    {
        if (ptr_)
            throw std::runtime_error("Resource already allocated");

        // 分配内存
        ptr_ = AllocTraits::allocate(alloc_, 1);

        try
        {
            // 构造对象
            AllocTraits::construct(alloc_, ptr_, std::forward<Args>(args)...);
        }
        catch (...)
        {
            // 分配失败时释放内存
            AllocTraits::deallocate(alloc_, ptr_, 1);
            ptr_ = nullptr;
            throw;
        }

        return ptr_;
    }

    // 异步释放资源
    void asyncRelease(std::chrono::milliseconds delay = std::chrono::milliseconds(10))
    {
        if (!ptr_ || released_)
            return;

        released_ = true;

        // 使用线程池或异步框架更合适，这里简化使用std::thread
        std::thread([this, delay]() {
            ptr_->doWork(); // NOTE: 异步工作启动
            std::this_thread::sleep_for(delay);

            // 确保在锁保护下执行析构和内存释放
            destroy();
        }).detach();
    }

    T *get() const
    {
        return ptr_;
    }

  private:
    void destroy()
    {
        if (!ptr_)
            return;

        // 析构对象
        AllocTraits::destroy(alloc_, ptr_);

        // 释放内存
        AllocTraits::deallocate(alloc_, ptr_, 1);

        ptr_ = nullptr;
    }
};

// 示例资源类
class NonCopyableResource
{
  public:
    NonCopyableResource()
    {
        std::cout << "Resource constructed" << std::endl;
    }
    ~NonCopyableResource()
    {
        std::cout << "Resource destroyed" << std::endl;
    }

    NonCopyableResource(const NonCopyableResource &) = delete;
    NonCopyableResource &operator=(const NonCopyableResource &) = delete;

    void doWork()
    {
        std::cout << "  >>> Working..." << std::endl;
    }
};

int main()
{
    AsyncResourceManager<NonCopyableResource> manager;

    // 创建资源
    NonCopyableResource *resource = manager.create();

    // 1秒后异步释放
    manager.asyncRelease();

    std::cout << "Main thread continues..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "main() done\n";
    return 0;
}
// NOLINTEND