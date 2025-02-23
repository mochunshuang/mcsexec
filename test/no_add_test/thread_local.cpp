#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <iostream>
#include <thread>

// NOLINTBEGIN

// 全局统计器用于验证线程安全
static inline std::atomic<uint64_t> total_allocated{0};
static inline std::atomic<uint64_t> total_deallocated{0};
static constexpr int THREAD_NUM = 100;
static constexpr int ITERATIONS = 1000;

struct MyClass
{
    MyClass()
    {
        if (total_allocated.load(std::memory_order_relaxed) % ITERATIONS == 0)
            std::cout << "MyClass constructed\n";
    }
    ~MyClass()
    {
        if (total_deallocated.load(std::memory_order_relaxed) % ITERATIONS == 0)
            std::cout << "MyClass destroyed\n";
    }
};

struct Manager
{
    struct BlockHeader
    {
        Manager *chunk;
        uint8_t index;
    };

    // 优化的参考：
    /**
       struct Chunk
    {

        using status_type = uint64_t;
        static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
        static constexpr size_t MAX_INDEX = BIT_COUNT - 1;
        static constexpr status_type INIT_STATUS = ~status_type{0};

        struct BlockHeader
        {
            Chunk *chunk;
            uint8_t index;
        };
        struct Block
        {
            alignas(alignof(BlockHeader)) std::byte data[sizeof(BlockHeader) + sizeof(T)];
        };

        // O(1) 删除的关键,配合 ResourcePool,
        Chunk *prev;
        Chunk *next;

        status_type status;
        std::array<Block, BIT_COUNT> blocks;// Note: 比 std::array<std::byte,
    (sizeof(BlockHeader) + sizeof(MyClass)) * 3> block; 设计差

        Chunk() noexcept : prev{}, next{}, status{INIT_STATUS} {}

        [[nodiscard]] index_t find_free_slot() const noexcept // NOLINT
        {
            return std::countl_zero(status);
        }

        T *allocate(const index_t &index) noexcept
        {
            assert((status & (index_t{1} << (MAX_INDEX - index))) != 0 &&
                   "The specified bit in status must be 1");

            status ^= (index_t{1} << (MAX_INDEX - index)); // NOLINTNEXTLINE
            BlockHeader &header = reinterpret_cast<BlockHeader &>(blocks[index].data);
            header.chunk = this;
            header.index = index; // NOLINTNEXTLINE
            return reinterpret_cast<T *>(blocks[index].data + sizeof(BlockHeader));
        }

        void deallocate(index_t index) noexcept
        {
            assert((status & (index_t{1} << (MAX_INDEX - index))) == 0 &&
                   "The specified bit in status must be 0");

            status = status & ~(index_t{1} << (MAX_INDEX - index));
        }
        [[nodiscard]] bool safe_destroy() const noexcept // NOLINT
        {
            return status == INIT_STATUS;
        }

        static Chunk *from_pointer(T *ptr) noexcept // NOLINT
        {
            // NOLINTNEXTLINE
            auto *header = reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(ptr) -
                                                           sizeof(BlockHeader));
            return header->chunk;
        }

        static uint8_t get_index(T *ptr) noexcept // NOLINT
        {
            // NOLINTNEXTLINE
            auto *header = reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(ptr) -
                                                           sizeof(BlockHeader));
            return header->index;
        }
    };
     */

    // TODO 封装 MyClass 为 链表节点，为了配合链表 O(1) 的操作申请和释放
    std::array<std::byte, (sizeof(BlockHeader) + sizeof(MyClass)) * 3> block;

    // TODO 优化  vector 为链表
    std::vector<uint8_t> available_indices = {0, 1, 2};

    MyClass *allocate()
    {
        if (available_indices.empty())
            return nullptr;

        uint8_t index = available_indices.back();
        available_indices.pop_back();

        size_t offset = index * (sizeof(BlockHeader) + sizeof(MyClass));

        BlockHeader *header = reinterpret_cast<BlockHeader *>(&block[offset]);
        header->chunk = this;
        header->index = index;

        MyClass *obj = reinterpret_cast<MyClass *>(&block[offset + sizeof(BlockHeader)]);
        std::construct_at(obj);

        return obj;
    }

    static void deallocate(MyClass *ptr)
    {
        if (!ptr)
            return;

        BlockHeader *header = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<std::byte *>(ptr) - sizeof(BlockHeader));

        Manager *manager = header->chunk;
        uint8_t index = header->index;

        std::destroy_at(ptr);
        manager->available_indices.push_back(index);
    }
};

void thread_test(int iterations)
{
    thread_local Manager local_manager; // 每个线程独立实例

    for (int i = 0; i < iterations; ++i)
    {
        MyClass *obj = local_manager.allocate();

        total_allocated++;

        // 模拟随机延迟（0~10ms）
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));

        Manager::deallocate(obj);
        total_deallocated++;
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

    std::cout << "\nTest Results:\n";
    std::cout << "Total allocated: " << total_allocated << '\n';
    std::cout << "Total deallocated: " << total_deallocated << '\n';

    // 验证内存平衡
    if (total_allocated == total_deallocated)
    {
        std::cout << "Memory management balanced\n";
    }
    else
    {
        std::cerr << "Memory leak detected!\n";
    }
    std::cout << "耗时: " << timer.elapsed() << " 秒" << std::endl;
}
// NOLINTEND