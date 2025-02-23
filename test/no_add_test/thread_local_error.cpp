#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cassert>

// 日志锁
std::mutex cout_mtx;
struct MyClass
{
    int value;
    MyClass() : value(42)
    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "Constructed on thread " << std::this_thread::get_id() << "\n";
    }
    ~MyClass()
    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "Destroyed on thread " << std::this_thread::get_id() << "\n";
    }
};

// 管理器定义（通过宏控制 Manager* 字段）
#define UNSAFE_MODE 1 // 设为 1 测试无 Manager* 时的行为

struct Manager
{
    struct Chunk;
    struct BlockHeader
    {
#if !UNSAFE_MODE
        Manager *manager; // 控制组保留，实验组移除
#endif
        Chunk *chunk;
        uint8_t index;
    };

    struct Chunk
    {
        uint8_t use_count = 0;
        void release()
        {
            assert(use_count > 0 && "Double free!");
            --use_count;
        }
    };

    // 全局释放函数
    static void static_deallocate(MyClass *ptr)
    {
        BlockHeader *header = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<char *>(ptr) - sizeof(BlockHeader));

#if UNSAFE_MODE
        // 实验组：直接操作 chunk（假设无跨线程问题）
        header->chunk->release();
#else
        // 控制组：通过 manager 路由
        header->manager->deallocate_impl(header->chunk);
#endif

        delete[] reinterpret_cast<char *>(header);
    }

#if !UNSAFE_MODE
    void deallocate_impl(Chunk *chunk)
    {
        chunk->release();
    }
#endif

    // 分配函数
    MyClass *allocate()
    {
        char *buffer = new char[sizeof(BlockHeader) + sizeof(MyClass)];
        BlockHeader *header = new (buffer) BlockHeader{
#if !UNSAFE_MODE
            this, // 控制组初始化 Manager*
#endif
            new Chunk(), 0};
        header->chunk->use_count = 1;
        return new (buffer + sizeof(BlockHeader)) MyClass();
    }

    ~Manager()
    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "Manager destroyed on thread " << std::this_thread::get_id() << "\n";
    }
};

// 线程任务：分配并立即释放（严格单线程操作）
void thread_task(int id)
{
    thread_local Manager pool; // 线程本地内存池
    MyClass *obj = pool.allocate();

    // 模拟正常使用
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 同一线程内释放
    Manager::static_deallocate(obj);
}

int main()
{
    constexpr int num_threads = 4;
    std::vector<std::jthread> threads;
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_task, i);
    }
    return 0;
}