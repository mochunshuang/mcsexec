#include <atomic>
#include <array>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <bit>

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

    struct Chunk
    {
        using status_type = uint64_t;
        static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
        static constexpr status_type INIT_STATUS = ~status_type{0};

        enum ListType
        {
            FREE,
            TMP,
            WAIT_DELETE,
            NONE
        };

        Chunk *prev = nullptr;
        Chunk *next = nullptr;
        status_type status = INIT_STATUS;
        uint8_t use_count = 0;
        ListType current_list = NONE;
        std::array<std::byte, (sizeof(BlockHeader) + sizeof(MyClass)) * BIT_COUNT> block;

        [[nodiscard]] uint8_t find_free_slot() const noexcept
        {
            if (status == 0)
                return BIT_COUNT;
            return std::countl_zero(status);
        }

        MyClass *allocate(uint8_t index) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status ^= mask;
            ++use_count;

            size_t offset = index * (sizeof(BlockHeader) + sizeof(MyClass));
            new (&block[offset]) BlockHeader{this, index};
            return new (&block[offset + sizeof(BlockHeader)]) MyClass();
        }

        void deallocate(uint8_t index) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status |= mask;
            --use_count;
        }
    };

    struct ChunkList
    {
        Chunk head;
        Chunk::ListType list_type;

        ChunkList(Chunk::ListType type) : list_type(type)
        {
            head.prev = &head;
            head.next = &head;
        }

        bool empty() const
        {
            return head.next == &head;
        }

        void push_front(Chunk *chunk)
        {
            chunk->prev = &head;
            chunk->next = head.next;
            head.next->prev = chunk;
            head.next = chunk;
            chunk->current_list = list_type;
        }

        void push_back(Chunk *chunk)
        {
            chunk->next = &head;
            chunk->prev = head.prev;
            head.prev->next = chunk;
            head.prev = chunk;
            chunk->current_list = list_type;
        }

        Chunk *pop_front()
        {
            if (empty())
                return nullptr;
            Chunk *chunk = head.next;
            erase(chunk);
            return chunk;
        }

        void erase(Chunk *chunk)
        {
            chunk->prev->next = chunk->next;
            chunk->next->prev = chunk->prev;
            chunk->prev = nullptr;
            chunk->next = nullptr;
            chunk->current_list = Chunk::NONE;
        }
    };

    ChunkList free_list{Chunk::FREE};
    ChunkList tmp_list{Chunk::TMP};
    ChunkList wait_delete_list{Chunk::WAIT_DELETE};

    MyClass *allocate()
    {
        if (auto chunk = free_list.pop_front())
        {
            if (uint8_t index = chunk->find_free_slot(); index < Chunk::BIT_COUNT)
            {
                MyClass *obj = chunk->allocate(index);
                if (chunk->use_count == Chunk::BIT_COUNT)
                {
                    tmp_list.push_front(chunk);
                }
                return obj;
            }
            free_list.push_front(chunk);
        }

        Chunk *new_chunk = new Chunk();
        free_list.push_front(new_chunk);
        return new_chunk->allocate(0);
    }

    void deallocate(MyClass *ptr) noexcept
    {
        BlockHeader *header = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<std::byte *>(ptr) - sizeof(BlockHeader));
        std::destroy_at(ptr);

        Chunk *chunk = header->chunk;
        const uint8_t index = header->index;
        chunk->deallocate(index);

        switch (chunk->current_list)
        {
        case Chunk::TMP:
            if (chunk->use_count == 0)
            {
                tmp_list.erase(chunk);
                wait_delete_list.push_front(chunk);
            }
            else if (chunk->use_count < Chunk::BIT_COUNT / 2)
            {
                tmp_list.erase(chunk);
                free_list.push_back(chunk);
            }
            break;
        default:
            break;
        }

        if (auto to_delete = wait_delete_list.pop_front())
        {
            delete to_delete;
        }
    }
    ~Manager() noexcept
    {
        // 辅助函数：删除链表中的所有Chunk
        auto delete_chunks = [](ChunkList &list) {
            while (Chunk *chunk = list.pop_front())
            {
                delete chunk;
            }
        };

        delete_chunks(free_list);
        delete_chunks(tmp_list);
        delete_chunks(wait_delete_list);
    }
};

void thread_test(int iterations)
{
    // 结论 ： 没有thread_local 更快
    Manager instance;
    for (int i = 0; i < iterations; ++i)
    {
        MyClass *obj = instance.allocate();
        total_allocated.fetch_add(1, std::memory_order_relaxed);

        // std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));

        instance.deallocate(obj);
        total_deallocated.fetch_add(1, std::memory_order_relaxed);
    }
}

// Timer类和main函数保持不变
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
    {
        std::cout << "new Manager::Chunk() start" << '\n';
        auto *p = new Manager::Chunk();
        std::cout << "new Manager::Chunk() end" << '\n';
        delete p;
    }
}
// NOLINTEND