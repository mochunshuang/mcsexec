#include <atomic>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <bit>
#include <type_traits>
#include <climits>

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

template <typename T>
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
        ListNode *prev{};
        ListNode *next{};
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
        ListNode list_node;

        static Chunk *from_list_node(ListNode *node) noexcept
        {
            return static_cast<Chunk *>(static_cast<void *>(node));
        }

        using status_type = uint64_t;
        using index_type = uint8_t;
        static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
        static constexpr status_type INIT_STATUS = ~status_type{0};
        static constexpr size_t BLOCK_ALIGN = std::max(alignof(BlockHeader), alignof(T));

        status_type status{INIT_STATUS};
        ListType current_list{NONE};
        alignas(BLOCK_ALIGN)
            std::array<std::byte, (sizeof(T) + sizeof(BlockHeader)) * BIT_COUNT> block;
        char padding[BIT_COUNT - (sizeof(ListNode) % BIT_COUNT)];

        static BlockHeader *get_header(T *ptr) noexcept
        {
            return reinterpret_cast<BlockHeader *>(reinterpret_cast<std::byte *>(ptr) +
                                                   sizeof(T));
        }

        [[nodiscard]] index_type find_free_slot() const noexcept
        {
            assert(status != 0);
            return std::countl_zero(status);
        }
        template <typename... Ags>
        T *allocate(index_type index, Ags &&...ags) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status ^= mask;
            size_t offset = index * (sizeof(T) + sizeof(BlockHeader));
            new (&block[offset]) T(std::forward<Ags>(ags)...);
            new (&block[offset + sizeof(T)]) BlockHeader{this, index};
            return reinterpret_cast<T *>(&block[offset]);
        }

        void deallocate(index_type index) noexcept
        {
            const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
            status |= mask;
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

    static_assert(offsetof(Chunk, list_node) == 0, "list_node must be the first member");
    static_assert(std::is_standard_layout_v<Chunk>, "Chunk must have standard layout");

    struct ChunkList
    {
        ListNode head; // 哨兵节点
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
            assert(chunk->current_list == NONE);
            ListNode *node = &chunk->list_node;
            node->prev = &head;
            node->next = head.next;
            head.next->prev = node;
            head.next = node;
            chunk->current_list = list_type;
        }

        void push_back(Chunk *chunk) noexcept
        {
            assert(chunk->current_list == ListType::NONE);
            ListNode *node = &chunk->list_node;
            node->next = &head;
            node->prev = head.prev;
            head.prev->next = node;
            head.prev = node;
            chunk->current_list = list_type;
        }

        [[nodiscard]] Chunk *header() const noexcept
        {
            if (empty())
                return nullptr;
            return Chunk::from_list_node(head.next);
        }

        Chunk *pop_front() noexcept
        {
            if (empty())
                return nullptr;
            ListNode *node = head.next;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->prev = node->next = nullptr;
            Chunk *chunk = Chunk::from_list_node(node);
            chunk->current_list = ListType::NONE; // 状态重置
            return chunk;
        }

        [[nodiscard]] bool contains(const Chunk *chunk) const noexcept
        {
            return chunk->current_list == list_type;
        }

        Chunk *remove(Chunk *chunk) noexcept
        {
            assert(chunk->current_list == list_type);

            // 执行链表解除操作
            ListNode *node = &chunk->list_node;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->prev = node->next = nullptr;

            chunk->current_list = ListType::NONE;
            return chunk;
        }
    };

    ChunkList free_list{ListType::FREE};
    ChunkList tmp_list{ListType::TMP};
    ChunkList wait_delete_list{ListType::WAIT_DELETE};
    uint64_t chunk_count{0};

    template <typename... Ags>
    T *allocate(Ags &&...ags)
    {
        // free_list 为空则 new / 保证一直有值
        if (free_list.header() == nullptr)
        {
            auto *new_chunk = new Chunk();
            free_list.push_front(new_chunk);
            chunk_count++;
            return new_chunk->allocate(0, std::forward<Ags>(ags)...);
        }

        auto *chunk = free_list.header();
        T *obj = chunk->allocate(chunk->find_free_slot(), std::forward<Ags>(ags)...);
        // 如果满了,提前移到tmp_list
        if (chunk->no_available())
        {
            tmp_list.push_front(free_list.pop_front());
        }
        return obj;
    }

    void deallocate(T *ptr) noexcept
    {
        BlockHeader *header = Chunk::get_header(ptr);

        Chunk *chunk = header->chunk;
        ptr->~T(); // 显式调用析构函数
        chunk->deallocate(header->index);

        switch (chunk->current_list)
        {
        case ListType::FREE:
            // free_list -> wait_delete_list
            if (chunk_count > 1 && chunk->all_available())
                wait_delete_list.push_back(free_list.remove(chunk));
            break;
        case ListType::TMP:
            // tmp_list -> wait_delete_list || free_list
            if (chunk_count > 1 && chunk->all_available())
                wait_delete_list.push_back(tmp_list.remove(chunk));
            else
                free_list.push_back(tmp_list.remove(chunk));
            break;
        case ListType::WAIT_DELETE:
        case ListType::NONE:
            std::abort(); // 不可能
            break;
        default:
            break;
        }

        // 为了 保证O(1)
        if (not wait_delete_list.empty())
        {
            chunk_count--;
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
        auto delete_chunks = [](ChunkList &list) noexcept {
            while (auto *chunk = list.pop_front())
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
    Manager<MyClass> instance;
    for (int i = 0; i < iterations; ++i)
    {
        MyClass *obj = instance.allocate();
        total_allocated.fetch_add(1, std::memory_order_relaxed);

        // std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));

        instance.deallocate(obj);
        total_deallocated.fetch_add(1, std::memory_order_relaxed);
    }
}
void thread_test2(int iterations)
{
    // 结论 ： 没有thread_local 更快
    thread_local Manager<MyClass> instance;
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

/**
 * @brief thread_local 平台性不太好？ clang 可用 g++ 不行。换个g++的源码路径了
 *
 * @return int
 */
void run_tests();
int main()
{
    double t0;
    double t1;
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

        t0 = timer.elapsed();

        std::cout << "耗时: " << timer.elapsed() << " 秒" << std::endl;
        std::cout << "\nTest Results:\n"
                  << "Total allocated: " << total_allocated << '\n'
                  << "Total deallocated: " << total_deallocated << '\n'
                  << (total_allocated == total_deallocated ? "Memory management balanced"
                                                           : "Memory leak detected")
                  << '\n';
    }
    {
        Timer timer;
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_NUM; ++i)
        {
            threads.emplace_back(thread_test2, ITERATIONS);
        }

        for (auto &t : threads)
        {
            t.join();
        }

        t1 = timer.elapsed();
    }

    {
        std::cout << "new Manager::Chunk() start" << '\n';
        auto *p = new Manager<MyClass>::Chunk();
        std::cout << "new Manager::Chunk() end" << '\n';
        delete p;
    }
    {
        std::cout << "new Manager::ChunkList() start" << '\n';
        auto *p = new Manager<MyClass>::ChunkList();
        assert(p->empty());
        {
            auto *node = new Manager<MyClass>::Chunk();
            p->push_front(node);
            (void)p->pop_front();
            delete node;
        }
        assert(p->empty());
        std::cout << "new Manager::ChunkList() end" << '\n';
        delete p;
    }
    {
        std::cout << "thread_local start" << '\n';
        auto lambda = [] {
            thread_local Manager<MyClass> instance;
            for (int i = 0; i < 1000; ++i)
            {
                MyClass *obj = instance.allocate();
                total_allocated.fetch_add(1, std::memory_order_relaxed);

                // std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));

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
    std::cout << "t0: " << t0 << "  t1: " << t1 << "  t0 < t1: " << (t0 < t1) << '\n';
    run_tests();
}

// 测试类1: 带虚函数的类型
class TestVirtual
{
  public:
    virtual ~TestVirtual() {} // 虚析构函数
    virtual int get_value() const
    {
        return 42;
    } // 虚方法
    int data = 0;
};

// 测试类2: 无虚函数的类型
class TestNormal
{
  public:
    int get_value() const
    {
        return 1337;
    }
    int data = 0;
};

// 内存布局验证工具函数
template <typename T>
void validate_memory_layout(Manager<T> &manager)
{
    T *obj = manager.allocate();
    auto *header = Manager<T>::Chunk::get_header(obj); // 关键修改点

    std::byte *t_end = reinterpret_cast<std::byte *>(obj) + sizeof(T);
    std::byte *header_start = reinterpret_cast<std::byte *>(header);
    assert(t_end == header_start);

    manager.deallocate(obj);
}

void test_virtual_function()
{
    Manager<TestVirtual> manager;
    TestVirtual *obj = manager.allocate();

    // 验证虚函数调用
    assert(obj->get_value() == 42);

    // 验证虚表指针存在
    void **vtable_ptr = reinterpret_cast<void **>(obj);
    assert(*vtable_ptr != nullptr);

    manager.deallocate(obj);
}

void test_allocation_cycle()
{
    const int TEST_COUNT = 1000;
    Manager<TestVirtual> manager;

    // 分配释放周期测试
    TestVirtual *objects[TEST_COUNT];
    for (int i = 0; i < TEST_COUNT; ++i)
    {
        objects[i] = manager.allocate();
        objects[i]->data = i;
    }

    // 验证数据完整性
    for (int i = 0; i < TEST_COUNT; ++i)
    {
        assert(objects[i]->data == i);
    }

    // 随机释放部分对象
    for (int i = 0; i < TEST_COUNT; i += 2)
    {
        manager.deallocate(objects[i]);
    }

    // 分配新对象验证复用
    for (int i = 0; i < TEST_COUNT; i += 2)
    {
        objects[i] = manager.allocate();
        assert(objects[i]->data == 0); // 新对象应初始化
    }

    // 清理全部
    for (int i = 0; i < TEST_COUNT; ++i)
    {
        manager.deallocate(objects[i]);
    }
}

void test_mixed_types()
{
    // 测试不同类型的管理器共存
    Manager<TestVirtual> manager1;
    Manager<TestNormal> manager2;

    auto *vobj = manager1.allocate();
    auto *nobj = manager2.allocate();

    assert(vobj->get_value() == 42);
    assert(nobj->get_value() == 1337);

    manager1.deallocate(vobj);
    manager2.deallocate(nobj);
}

// 添加子类定义
class TestDerived : public TestVirtual
{
  public:
    ~TestDerived() override
    {
        destroyed_count++;
    }
    int get_value() const override
    {
        return 100;
    }
    static inline int destroyed_count = 0;
};

// 测试子类析构函数调用
void test_subclass_destructor()
{
    TestDerived::destroyed_count = 0;

    {
        Manager<TestDerived> manager;
        TestDerived *obj = manager.allocate();
        assert(obj->get_value() == 100);
        manager.deallocate(obj);
    }

    assert(TestDerived::destroyed_count == 1);
    std::cout << "Subclass destructor test passed.\n";
}

// 测试子类内存布局
void test_subclass_memory_layout()
{
    Manager<TestDerived> manager;
    validate_memory_layout(manager);
    std::cout << "Subclass memory layout test passed.\n";
}

void run_tests()
{
    // 基本功能测试
    test_virtual_function();

    // 内存布局验证
    Manager<TestVirtual> vmgr;
    validate_memory_layout(vmgr);

    Manager<TestNormal> nmgr;
    validate_memory_layout(nmgr);

    // 压力测试
    test_allocation_cycle();

    // 混合类型测试
    test_mixed_types();

    // 新增子类测试
    test_subclass_destructor();
    test_subclass_memory_layout();

    {
        using T = TestVirtual;
        Manager<T> pool;
        T *p1 = pool.allocate();
        pool.deallocate(p1);
        T *p2 = pool.allocate();       // 可能复用同一地址
        assert(p1 == p2);              // 假设内存池复用
        assert(p2->get_value() == 42); // 若无 launder，此处是否正常？
    }

    std::cout << "All tests passed!\n";
}