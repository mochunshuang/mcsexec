#include <atomic>
#include <cassert>
#include <climits>
#include <cstdint>
#include <exception>
#include <iostream>
#include <array>
#include <thread>
#include <vector>

template <typename T>
class ResourcePool
{
    T *m_head = nullptr;
    T *m_tail = nullptr;

  public:
    T *header()
    {
        return m_head;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_head == nullptr;
    }

    void append_front(T *obj) noexcept // NOLINT
    {
        assert(obj != nullptr);
        assert(obj->prev == nullptr && obj->next == nullptr);
        assert(obj->owning_pool == nullptr); // 确保节点未属于其他池

        obj->owning_pool = this; // 设置所属池

        obj->next = m_head;
        obj->prev = nullptr;
        if (m_head)
        {
            m_head->prev = obj;
        }
        else
        {
            m_tail = obj;
        }
        m_head = obj;
    }

    void append_back(T *obj) noexcept // NOLINT
    {
        assert(obj != nullptr);
        assert(obj->prev == nullptr && obj->next == nullptr);
        assert(obj->owning_pool == nullptr);
        obj->owning_pool = this;

        obj->prev = m_tail;
        obj->next = nullptr;
        if (m_tail)
        {
            m_tail->next = obj;
        }
        else
        {
            m_head = obj;
        }
        m_tail = obj;
    }

    bool remove(T *obj) noexcept
    {
        assert(obj != nullptr);
        assert(obj->owning_pool == this); // 验证节点属于当前池
        obj->owning_pool = nullptr;       // 重置所属池

        // 更新前驱节点的 next 指针
        if (obj->prev)
        {
            obj->prev->next = obj->next;
        }
        else
        {
            m_head = obj->next; // 若删除的是头节点，更新头指针
        }

        // 更新后继节点的 prev 指针
        if (obj->next)
        {
            obj->next->prev = obj->prev;
        }
        else
        {
            m_tail = obj->prev; // 若删除的是尾节点，更新尾指针
        }

        // 清除被删除节点的指针
        obj->prev = obj->next = nullptr;
        return true;
    }

    ~ResourcePool() noexcept
    {
        T *current = m_head;
        while (current != nullptr)
        {
            assert(current->owning_pool == this);

            T *next = current->next;
            assert(next == nullptr || next->prev == current); // 验证双向链表
            current->owning_pool = nullptr;                   // 解除关联，避免悬空指针
            operator delete(current);
            current = next;
        }
        m_head = m_tail = nullptr; // 显式清空
    }

    ResourcePool() = default;
    ResourcePool(const ResourcePool &) = delete;
    ResourcePool &operator=(const ResourcePool &) = delete;
    ResourcePool(ResourcePool &&) = delete;
    ResourcePool &operator=(ResourcePool &&) = delete;
};

template <typename T>
struct FreeList
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
        static constexpr double RATIO_THRESHOLD = 0.5;
        struct Block
        {
            alignas(alignof(BlockHeader)) std::byte data[sizeof(BlockHeader) + sizeof(T)];
        };
        enum class PoolType : int8_t
        {
            FREE,
            FULL,
            WAIT_DESTROY
        };

        FreeList &pool;
        // O(1) 删除的关键,配合 ResourcePool,
        Chunk *prev = nullptr;
        Chunk *next = nullptr;

        status_type status = INIT_STATUS;
        std::array<Block, BIT_COUNT> blocks;
        PoolType current_pool = PoolType::FREE;
        ResourcePool<Chunk> *owning_pool = nullptr; // 新增所属池指针
        explicit Chunk(FreeList &p) : pool(p) {}

        Chunk() = default;
        Chunk(const Chunk &) = delete;
        Chunk &operator=(const Chunk &) = delete;
        Chunk(Chunk &&) = delete;
        Chunk &operator=(Chunk &&) = delete;
        ~Chunk() noexcept = default;

        [[nodiscard]] uint8_t find_free_slot() const noexcept
        {
            return std::countl_zero(status);
        }

        T *allocate(uint8_t index) noexcept
        {
            status ^= (1ULL << (BIT_COUNT - 1 - index));
            pool.used_count++;

            BlockHeader *header = reinterpret_cast<BlockHeader *>(blocks[index].data);
            header->chunk = this;
            header->index = index;

            may_change_pool(true);
            return reinterpret_cast<T *>(blocks[index].data + sizeof(BlockHeader));
        }

        void deallocate(uint8_t index) noexcept
        {
            status |= (1ULL << (BIT_COUNT - 1 - index));
            pool.used_count--;

            may_change_pool(false);
        }
        [[nodiscard]] bool safe_destroy() const noexcept
        {
            return status == INIT_STATUS;
        }

        static Chunk *from_pointer(T *ptr) noexcept
        {
            auto *header = reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(ptr) -
                                                           sizeof(BlockHeader));
            return header->chunk;
        }

        static uint8_t get_index(T *ptr) noexcept
        {
            auto *header = reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(ptr) -
                                                           sizeof(BlockHeader));
            return header->index;
        }

        [[nodiscard]] bool need_full_to_free() const noexcept
        {
            if (pool.total_count < BIT_COUNT * 2)
                return true;

            auto ratio = static_cast<double>(pool.used_count) / pool.total_count;
            return ratio > RATIO_THRESHOLD;
        }

        // free_pool <=> full_pool
        // full_pool ->  wait_destroy_pool
        void may_change_pool(bool is_allocate) noexcept // NOLINT
        {
            if (is_allocate)
            {
                if (current_pool == PoolType::FREE && status == 0)
                {
                    // 从 free_pool 移除并加入 full_pool
                    assert(owning_pool == &pool.free_pool);
                    pool.free_pool.remove(this);
                    pool.full_pool.append_back(this);
                    current_pool = PoolType::FULL;
                }
            }
            else
            {
                if (current_pool == PoolType::FULL)
                {
                    if (need_full_to_free())
                    {
                        assert(owning_pool == &pool.full_pool);
                        pool.full_pool.remove(this);
                        pool.free_pool.append_back(this);
                        current_pool = PoolType::FREE;
                    }
                    else if (status == INIT_STATUS)
                    {
                        assert(owning_pool == &pool.full_pool);
                        pool.full_pool.remove(this);
                        pool.wait_destroy_pool.append_back(this);
                        current_pool = PoolType::WAIT_DESTROY;
                    }
                }
            }
        }
    };

    ResourcePool<Chunk> free_pool{};
    ResourcePool<Chunk> full_pool{};
    ResourcePool<Chunk> wait_destroy_pool{};

    std::size_t total_count{0};
    std::size_t used_count{0};

    template <typename... Args>
    T *borrowing(Args &&...args)
    {
        Chunk *chunk = free_pool.header();
        if (chunk == nullptr)
        {
            chunk = new Chunk(*this);
            total_count += Chunk::BIT_COUNT;
            chunk->current_pool = Chunk::PoolType::FREE;
            free_pool.append_front(chunk);
        }

        const uint8_t index = chunk->find_free_slot(); // NOLINT
        assert(index < Chunk::BIT_COUNT);
        return new (chunk->allocate(index)) T(std::forward<Args>(args)...);
    }

    void send_back(T *ptr) noexcept
    {
        ptr->~T();

        Chunk *chunk = Chunk::from_pointer(ptr);
        const uint8_t index = Chunk::get_index(ptr); // NOLINT
        chunk->deallocate(index);

        // 缩容操作
        static constexpr auto k_count = Chunk::BIT_COUNT * 2;
        // 缩容时确保节点已从所有池中移除
        if (total_count > k_count && !wait_destroy_pool.empty())
        {
            Chunk *current = wait_destroy_pool.header();
            while (current != nullptr)
            {
                Chunk *next = current->next;
                if (total_count <= k_count)
                    break;
                if (current->safe_destroy())
                {
                    // 确保节点属于 wait_destroy_pool
                    assert(current->owning_pool == &wait_destroy_pool);
                    bool ret = wait_destroy_pool.remove(current);
                    assert(ret && current->safe_destroy());

                    total_count -= Chunk::BIT_COUNT;
                    delete current; // 安全删除
                }

                current = next;
            }
        }
    }
};

struct MyObject
{
    int data[4];
    MyObject(int a = 0, int b = 0, int c = 0, int d = 0) noexcept : data{a, b, c, d}
    {
        std::cout << "MyObject()\n";
    }
    ~MyObject()
    {
        std::cout << "~MyObject()\n";
    }
};

void base_test0();
void base_test1();
int main()
{
    FreeList<MyObject> pool;

    MyObject *obj1 = pool.borrowing(); // 应该是 blocks[0]
    MyObject *obj2 = pool.borrowing(); // 应该是 blocks[1]

    std::cout << "================\n";
    // Note:  pool.borrowing() +  外部，说明这里调用两次new 释放合理
    // return new (chunk->allocate(index)) T(std::forward<Args>(args)...);
    new (obj1) MyObject{1, 2, 3, 4};
    new (obj2) MyObject{5, 6, 7, 8};

    pool.send_back(obj1); // 归还 blocks[0]
    pool.send_back(obj2); // 归还 blocks[1]

    std::cout << "================\n";
    {
        MyObject *obj3 = pool.borrowing(); // 应该得到 blocks[0]
        assert((obj3 == obj1 || obj3 == obj2) && "Memory not reused");
        assert((obj3 == obj1) && "obj3 == obj1 error");
        pool.send_back(obj3);
        //
        std::cout << "here\n";
        // 疑惑： send_back 会调用 obj的 析构。会不会 破坏
        new (obj3) MyObject{9, 10, 11, 12};
        assert(obj3->data[3] == 12);
        pool.send_back(obj3);
    }
    std::cout << "================\n";
    {
        // 避免两次构造。直接返回对象
        MyObject *obj3 = pool.borrowing(9, 10, 11, 12); // 应该得到 blocks[0]
        assert((obj3 == obj1) && "obj3 == obj1 error");
        assert(obj3->data[3] == 12);
        pool.send_back(obj3);
    }

    base_test0(); // NOLINT
    base_test1();
    std::cout << "\nall pass\n";
    return 0;
}

struct TestObject
{
    int value;
    TestObject(int v) : value(v) {}
};

void test_basic_allocation()
{
    thread_local FreeList<TestObject> pool;

    // 分配一个对象
    TestObject *obj1 = pool.borrowing(42);
    assert(obj1->value == 42);
    assert(pool.used_count == 1);

    // 释放对象
    pool.send_back(obj1);
    assert(pool.used_count == 0);

    // 重新分配，应复用内存
    TestObject *obj2 = pool.borrowing(100);
    assert(obj2->value == 100);
    assert(obj2 == obj1); // 验证内存复用
    pool.send_back(obj2);
}

void test_expansion()
{
    thread_local FreeList<TestObject> pool;
    const size_t chunk_capacity = FreeList<TestObject>::Chunk::BIT_COUNT;

    // 分配超过一个 Chunk 的容量
    std::vector<TestObject *> objects;
    for (size_t i = 0; i < chunk_capacity + 5; ++i)
    {
        objects.push_back(pool.borrowing(i));
    }
    assert(pool.total_count >= 2 * chunk_capacity); // 验证扩容

    // 释放所有对象
    for (auto *obj : objects)
    {
        pool.send_back(obj);
    }
}

void test_shrink()
{
    thread_local FreeList<TestObject> pool;
    const size_t chunk_capacity = FreeList<TestObject>::Chunk::BIT_COUNT;

    // 分配大量对象后释放，触发缩容
    std::vector<TestObject *> objects;
    for (size_t i = 0; i < 3 * chunk_capacity; ++i)
    {
        objects.push_back(pool.borrowing(i));
    }
    size_t initial_total = pool.total_count;

    // 释放对象，触发缩容
    for (auto *obj : objects)
    {
        pool.send_back(obj);
    }
    assert(pool.total_count < initial_total); // 验证缩容
}

void base_test0()
{
    test_basic_allocation();
    test_expansion();
    test_shrink();
    std::cout << "All single-threaded tests passed!" << '\n';
}

void thread_task(int id)
{
    // thread_local FreeList<TestObject> pool;
    FreeList<TestObject> pool;

    // 每个线程分配和释放对象
    const int num_objects = 100;
    std::vector<TestObject *> objects;
    for (int i = 0; i < num_objects; ++i)
    {
        objects.push_back(pool.borrowing(id * 1000 + i));
    }

    // 确保无冲突
    for (auto *obj : objects)
    {
        assert(obj->value >= id * 1000 && obj->value < (id + 1) * 1000);
        pool.send_back(obj);
    }
}

void test_thread_safety()
{
    const int num_threads = 4;
    std::vector<std::jthread> threads;
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([]() {
            // thread_local 一坨屎
            FreeList<TestObject> pool;
            for (int j = 0; j < 1000; ++j)
            {
                auto *obj = pool.borrowing(j);
                pool.send_back(obj);
            }
        });
    }
    for (auto &t : threads)
    {
        t.join();
    }
    std::cout << "Multi-threaded test passed!" << '\n';
}
void test_thread_safety2()
{
    const int num_threads = 4;
    std::vector<std::jthread> threads;
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_task, i);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    std::cout << "Multi-threaded test passed!" << '\n';
}

void test_destructor_safety()
{
    {
        thread_local FreeList<TestObject> pool; // 确保线程本地存储
        auto *obj = pool.borrowing(42);
        pool.send_back(obj);
    }
}

void test_destructor_safety2()
{
    std::jthread t([]() {
        {
            FreeList<TestObject> pool;
            auto *obj = pool.borrowing(100);
            pool.send_back(obj);
        } // 池在此处析构，早于线程结束
    });
    t.join();
}

void base_test1()
{
    std::cout << "test_destructor_safety! start" << std::endl;
    for (int i = 0; i < 1000; i++) // NOLINT
    {
        test_destructor_safety();
    }
    std::cout << "test_destructor_safety! pass" << std::endl;

    {
        std::cout << "test_destructor_safety2! start" << std::endl;
        for (int i = 0; i < 1000; i++) // NOLINT
        {
            test_destructor_safety2();
        }
        std::cout << "test_destructor_safety2! pass" << std::endl;
    }
    test_thread_safety();
    test_thread_safety2();
}