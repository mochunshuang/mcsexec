#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

struct MyClass
{
    int value; // 添加成员变量以增大内存操作风险
    MyClass() : value(42)
    {
        std::cout << "Constructed on thread " << std::this_thread::get_id() << "\n";
    }
    ~MyClass()
    {
        std::cout << "Destroyed on thread " << std::this_thread::get_id() << "\n";
    }
};

// 管理器定义（危险版本：BlockHeader 不含 Manager*）
struct Manager
{
    struct Chunk;
    struct BlockHeader
    {
        Chunk *chunk; // 缺少 Manager* 字段
        uint8_t index;
    };

    struct Chunk
    {
        uint8_t use_count = 0;
        void release()
        {
            // 模拟真实内存池操作：修改 Chunk 状态
            assert(use_count > 0 && "Double free detected!");
            --use_count;
        }
    };

    // 全局释放函数（无法感知 thread_local）
    static void static_deallocate(MyClass *ptr)
    {
        BlockHeader *header = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<char *>(ptr) - sizeof(BlockHeader));
        header->chunk->release();                  // 危险：可能操作已销毁的 Chunk
        delete[] reinterpret_cast<char *>(header); // 释放整个内存块
    }

    // 分配函数
    MyClass *allocate()
    {
        char *buffer = new char[sizeof(BlockHeader) + sizeof(MyClass)];
        BlockHeader *header = new (buffer) BlockHeader{new Chunk(), 0};
        header->chunk->use_count = 1; // 标记 Chunk 使用中
        return new (buffer + sizeof(BlockHeader)) MyClass();
    }

    ~Manager()
    {
        // 析构时释放所有 Chunk（模拟内存池清理）
        std::cout << "Manager destroyed on thread " << std::this_thread::get_id() << "\n";
    }
};

// 线程本地 Manager
thread_local Manager local_manager;

// 子线程函数：分配对象但不释放
void thread_func(MyClass **obj_ptr)
{
    *obj_ptr = local_manager.allocate(); // 由子线程分配
}

int main()
{
    MyClass *cross_thread_obj = nullptr;

    // 启动子线程分配对象
    std::thread t([&] { thread_func(&cross_thread_obj); });
    t.join(); // 等待子线程结束（此时子线程的 Manager 已销毁）

    // 主线程尝试释放子线程创建的对象（危险！）
    Manager::static_deallocate(cross_thread_obj); // 触发崩溃点

    return 0;
}