#include <array>
#include <atomic>
#include <bit>
#include <iostream>
#include <ostream>
#include <thread>
#include <vector>

// NOLINTBEGIN
static inline std::atomic<uint64_t> total_allocated{0};
static inline std::atomic<uint64_t> total_deallocated{0};
static constexpr int THREAD_NUM = 100;
static constexpr int ITERATIONS = 1000;

struct MyClass
{
    MyClass() noexcept
    {
        if (auto times = total_allocated.load(std::memory_order_relaxed);
            times % ITERATIONS == 0)
            std::cout << "MyClass constructed\n";
    }
    ~MyClass() noexcept
    {
        if (auto times = total_deallocated.load(std::memory_order_relaxed);
            times % ITERATIONS == 0)
            std::cout << "MyClass destroyed\n";
    }
};

struct Manager
{
    struct Chunk;
    struct BlockHeader
    {
        Chunk *chunk;
        uint8_t index;
    };

    // 内存块元数据
    struct Chunk
    {
        using status_type = uint64_t;
        static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
        static constexpr status_type INIT_STATUS = ~status_type{0};

        Chunk *prev = nullptr;
        Chunk *next = nullptr;
        status_type status = INIT_STATUS;
        std::array<std::byte, (sizeof(BlockHeader) + sizeof(MyClass)) * BIT_COUNT> block;

        Chunk() = default;

        // 查找第一个可用槽位
        [[nodiscard]] uint8_t find_free_slot() const noexcept
        {
            if (status == 0)
                return BIT_COUNT;
            return std::countl_zero(status);
        }

        // 分配指定槽位
        MyClass *allocate(uint8_t index) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status ^= mask;

            // 计算内存偏移（显式对齐）
            static constexpr size_t block_size = sizeof(BlockHeader) + sizeof(MyClass);
            size_t offset = index * block_size;

            // 填写BlockHeader
            new (&block[offset]) BlockHeader{this, index};
            return new (&block[offset + sizeof(BlockHeader)]) MyClass();
        }

        // 释放指定槽位
        void deallocate(uint8_t index) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status |= mask;
        }
    };

    // 链表管理
    Chunk head;
    Chunk *current = &head;

    MyClass *allocate()
    {

        // 查找当前块可用槽位
        if (uint8_t index = current->find_free_slot(); index < Chunk::BIT_COUNT)
        {
            return current->allocate(index);
        }

        // 分配新块
        Chunk *new_chunk = new Chunk();
        new_chunk->prev = current;
        current->next = new_chunk;
        current = new_chunk;
        return new_chunk->allocate(0);
    }

    static void deallocate(MyClass *ptr) noexcept
    {
        BlockHeader *header = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<std::byte *>(ptr) - sizeof(BlockHeader));

        // 显式析构对象
        std::destroy_at(ptr); // 关键修复：显式析构对象

        auto *chunk = header->chunk;
        chunk->deallocate(header->index);

        // 回收空块（当块完全空闲且不是头节点）
        if (chunk->status == Chunk::INIT_STATUS && chunk->prev)
        {
            chunk->prev->next = chunk->next;
            if (chunk->next)
                chunk->next->prev = chunk->prev;
            delete chunk;
        }
    }
};

void thread_test(int iterations)
{
    thread_local Manager local_manager;

    for (int i = 0; i < iterations; ++i)
    {
        MyClass *obj = local_manager.allocate();
        total_allocated.fetch_add(1, std::memory_order_relaxed);

        // std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));

        Manager::deallocate(obj);
        total_deallocated.fetch_add(1, std::memory_order_relaxed);
    }
}

class Timer
{
  public:
    Timer() : start_time(std::chrono::steady_clock::now()) {}

    void reset()
    {
        start_time = std::chrono::steady_clock::now();
    }

    double elapsed() const
    {
        auto end_time = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end_time - start_time).count();
    }

  private:
    std::chrono::steady_clock::time_point start_time;
};

int main()
{
    Timer timer;
    std::vector<std::thread> threads;
    for (int i = 0; i < THREAD_NUM; ++i)
    {
        threads.emplace_back(thread_test, ITERATIONS);
    }

    for (auto &t : threads)
    {
        t.join();
    }

    std::cout << "耗时: " << timer.elapsed() << " 秒" << std::endl;
    std::cout << "\nTest Results:\n"
              << "Total allocated: " << total_allocated << '\n'
              << "Total deallocated: " << total_deallocated << '\n'
              << (total_allocated == total_deallocated ? "Memory management balanced"
                                                       : "Memory leak detected")
              << '\n';
}
// NOLINTEND