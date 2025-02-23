#include <atomic>
#include <array>
#include <cassert>
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
    enum ListType
    {
        FREE,
        TMP,
        WAIT_DELETE,
        NONE
    };

    struct Chunk
    {
        using status_type = uint64_t;
        static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
        static constexpr status_type INIT_STATUS = ~status_type{0};

        Chunk *prev = nullptr;
        Chunk *next = nullptr;
        status_type status = INIT_STATUS;
        uint8_t use_count = 0;
        ListType current_list = NONE;
        static constexpr size_t BLOCK_ALIGN =
            std::max(alignof(BlockHeader), alignof(MyClass));
        alignas(BLOCK_ALIGN) std::array<
            std::byte, (sizeof(BlockHeader) + sizeof(MyClass)) * BIT_COUNT> block;

        [[nodiscard]] uint8_t find_free_slot() const noexcept
        {
            if (status == 0)
                return BIT_COUNT;
            return std::countl_zero(status);
        }

        MyClass *allocate(uint8_t index) noexcept
        {
            assert(index < BIT_COUNT);
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
        [[nodiscard]] bool no_available() const noexcept
        {
            return status == 0;
        }
        [[nodiscard]] bool all_available() const noexcept
        {
            return status == INIT_STATUS;
        }
    };

    struct ChunkList
    {
        Chunk head{};
        ListType list_type;
        std::size_t size{0};

        ChunkList(ListType type) : list_type(type)
        {
            head.prev = &head;
            head.next = &head;
        }

        bool empty() const
        {
            return size == 0;
        }

        void push_front(Chunk *chunk)
        {
            assert(chunk != nullptr);
            assert(chunk->current_list == ListType::NONE);
            assert(chunk != &head); // 禁止插入哨兵节点

            chunk->prev = &head;
            chunk->next = head.next;
            head.next->prev = chunk;
            head.next = chunk;
            chunk->current_list = list_type;
            ++size; // 确保 size 与链表实际节点数一致
        }

        void push_back(Chunk *chunk)
        {
            assert(chunk->current_list == ListType::NONE);
            chunk->next = &head;
            chunk->prev = head.prev;
            head.prev->next = chunk;
            head.prev = chunk;
            chunk->current_list = list_type;
            ++size;
        }

        Chunk *pop_front() noexcept
        {
            if (empty())
                return nullptr;
            Chunk *chunk = head.next;
            return remove(chunk);
        }

        Chunk *remove(Chunk *chunk) noexcept
        {
            if (chunk == nullptr || chunk == &head)
            {
                return nullptr; // 防止操作哨兵节点
            }

            // 从链表中移除节点
            chunk->prev->next = chunk->next;
            chunk->next->prev = chunk->prev;
            // 重置节点指针
            chunk->prev = nullptr;
            chunk->next = nullptr;
            chunk->current_list = ListType::NONE;
            --size; // 确保 size 减少
            return chunk;
        }

        [[nodiscard]] Chunk *header() const noexcept
        {
            if (empty())
                return nullptr;
            return head.next;
        }
        ~ChunkList() noexcept
        {
            if (size < 1)
                return;

            while (Chunk *current = pop_front())
            {

                // 删除当前节点
                delete current;
            }
            size = 0;
        }
    };

    ChunkList free_list{ListType::FREE};
    ChunkList tmp_list{ListType::TMP};
    ChunkList wait_delete_list{ListType::WAIT_DELETE};
    uint64_t chunk_count{0};

    MyClass *allocate()
    {
        // free_list 为空则 new / 保证一直有值
        if (free_list.header() == nullptr)
        {
            auto *new_chunk = new Chunk();
            free_list.push_front(new_chunk);
            chunk_count++;
            return new_chunk->allocate(0);
        }

        auto *chunk = free_list.header();
        MyClass *obj = chunk->allocate(chunk->find_free_slot());
        // 如果满了,提前移到tmp_list
        if (chunk->no_available())
        {
            tmp_list.push_front(free_list.pop_front());
        }
        return obj;
    }

    void deallocate(MyClass *ptr) noexcept
    {

        BlockHeader *header = reinterpret_cast<BlockHeader *>(
            reinterpret_cast<std::byte *>(ptr) - sizeof(BlockHeader));
        std::destroy_at(ptr);

        Chunk *chunk = header->chunk;
        const uint8_t index = header->index;
        chunk->deallocate(index);

        assert(chunk->current_list != NONE);

        // 恢复链表迁移逻辑
        switch (chunk->current_list)
        {
        case ListType::FREE:
            if (chunk->all_available() && chunk_count > 2)
            {
                wait_delete_list.push_back(free_list.remove(chunk));
            }
            break;
        case ListType::TMP:
            if (chunk->all_available() && chunk_count > 2)
            {
                wait_delete_list.push_back(tmp_list.remove(chunk));
            }
            else
            {
                free_list.push_back(tmp_list.remove(chunk));
            }
            break;
        default:
            std::abort(); // 处理非法状态
        }

        // 立即删除可释放的 Chunk
        if (auto *to_delete = wait_delete_list.pop_front())
        {
            delete to_delete;
            --chunk_count; // 维护 chunk_count
        }
    }
};

void thread_test(int iterations)
{
    // thread_local Manager instance;
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
    // NOTE: 拼尽全力，依然无法战胜。 https://www.onlinegdb.com/ 没问题，clang++的问题？
    // Note: 在线编译器：gcc,clang 都能过： https://wandbox.org/
    // Note: 只能承认自己c++编译器有毛病了. 本地clang++ 没问题，尼玛的
#if 1
    {
        {
            std::cout << "thread_local start" << '\n';
            auto lambda = [] {
                // thread_local Manager instance;
                thread_local Manager instance;
                // Manager instance;
                for (int i = 0; i < 1000; ++i)
                {
                    MyClass *obj = instance.allocate();
                    total_allocated.fetch_add(1, std::memory_order_relaxed);

                    // std::this_thread::sleep_for(std::chrono::milliseconds(rand() %
                    // 10));

                    instance.deallocate(obj);
                    total_deallocated.fetch_add(1, std::memory_order_relaxed);
                }
            };
            std::vector<std::thread> threads;
            for (int i = 0; i < THREAD_NUM; ++i)
            {
                threads.emplace_back(lambda);
            }
            for (auto &t : threads)
            {
                t.join();
            }
            std::cout << "thread_local end" << '\n';
        }
    }
#endif
}
// NOLINTEND