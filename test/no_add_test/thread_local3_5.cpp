#include <atomic>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <bit>

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

    // 侵入式链表节点
    struct ListNode
    {
        ListNode *prev = nullptr;
        ListNode *next = nullptr;
    };
    enum ListType : std::uint8_t
    {
        FREE,
        TMP,
        WAIT_DELETE,
        NONE
    };

    struct Chunk
    {
        // 链表节点必须作为第一个成员
        ListNode list_node;
        static Chunk *from_list_node(ListNode *node) noexcept
        {
            // TODO(mcs): 如果有虚函数，会不会崩溃
            //  利用list_node作为首成员的特性
            return reinterpret_cast<Chunk *>(node);
        }

        using status_type = uint64_t;
        static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
        static constexpr status_type INIT_STATUS = ~status_type{0};

        status_type status = INIT_STATUS;
        uint8_t use_count = 0;
        ListType current_list = NONE;

        alignas(alignof(BlockHeader)) std::array<
            std::byte, (sizeof(BlockHeader) + sizeof(MyClass)) * BIT_COUNT> block;

        static BlockHeader *get_header(MyClass *ptr) noexcept
        {
            return reinterpret_cast<BlockHeader *>(reinterpret_cast<std::byte *>(ptr) -
                                                   sizeof(BlockHeader));
        }

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
            static constexpr size_t base_offset = sizeof(BlockHeader) + sizeof(MyClass);
            size_t offset = index * base_offset;
            new (&block[offset]) BlockHeader{.chunk = this, .index = index};
            return new (&block[offset + sizeof(BlockHeader)]) MyClass();
        }

        void deallocate(const uint8_t &index) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status |= mask;
            --use_count;
        }
    };

    struct ChunkList
    {
        ListNode head;
        ListType list_type;

        explicit ChunkList(ListType type = NONE) : list_type(type)
        {
            head.prev = &head;
            head.next = &head;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return head.next == &head;
        }

        void push_front(Chunk *chunk) noexcept
        {
            ListNode *node = &chunk->list_node;
            node->prev = &head;
            node->next = head.next;
            head.next->prev = node;
            head.next = node;
            chunk->current_list = list_type;
        }

        void push_back(Chunk *chunk) noexcept
        {
            ListNode *node = &chunk->list_node;
            node->next = &head;
            node->prev = head.prev;
            head.prev->next = node;
            head.prev = node;
            chunk->current_list = list_type;
        }

        [[nodiscard]] Chunk *pop_front() const noexcept
        {
            if (empty())
                return nullptr;
            ListNode *node = head.next;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->prev = node->next = nullptr;
            Chunk *chunk = Chunk::from_list_node(node);
            chunk->current_list = ListType::NONE;
            return chunk;
        }

        static void erase(Chunk *chunk) noexcept
        {
            ListNode *node = &chunk->list_node;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->prev = node->next = nullptr;
            chunk->current_list = ListType::NONE;
        }
    };

    ChunkList free_list{ListType::FREE};
    ChunkList tmp_list{ListType::TMP};
    ChunkList wait_delete_list{ListType::WAIT_DELETE};

    MyClass *allocate()
    {
        // TODO(mcs): 又是 pop_front 又是 push_front。感觉设计有问题
        if (auto *chunk = free_list.pop_front())
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

        auto *new_chunk = new Chunk();
        free_list.push_front(new_chunk);
        return new_chunk->allocate(0);
    }

    void deallocate(MyClass *ptr) noexcept
    {
        BlockHeader *header = Chunk::get_header(ptr);

        Chunk *chunk = header->chunk;
        chunk->deallocate(header->index);

        switch (chunk->current_list)
        {
        case ListType::TMP:
            if (chunk->use_count == 0)
            {
                Manager::ChunkList::erase(chunk);
                wait_delete_list.push_front(chunk);
            }
            else if (chunk->use_count < Chunk::BIT_COUNT / 2)
            {
                Manager::ChunkList::erase(chunk);
                free_list.push_back(chunk);
            }
            break;
        default:
            break;
        }

        if (not wait_delete_list.empty())
        {
            delete wait_delete_list.pop_front();
        }
    }
    Manager() = default;
    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;
    Manager(Manager &&) = delete;
    Manager &operator=(Manager &&) = delete;
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

    [[nodiscard]] double elapsed() const
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
    {
        std::cout << "new Manager::ChunkList() start" << '\n';
        auto *p = new Manager::ChunkList();
        assert(p->empty());
        {
            auto *node = new Manager::Chunk();
            p->push_front(node);
            p->pop_front();
            delete node;
        }
        assert(p->empty());
        std::cout << "new Manager::ChunkList() end" << '\n';
        delete p;
    }
}