#include <cstddef>
#include <math.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <forward_list>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_set>

class TrackedClass // NOLINT
{
  public:
    TrackedClass()
    {
        ++constructed;
    }
    ~TrackedClass()
    {
        ++destructed;
    }
    static std::atomic<int> constructed;
    static std::atomic<int> destructed;
    static void reset()
    {
        constructed = 0;
        destructed = 0;
    }
};

std::atomic<int> TrackedClass::constructed = 0;
std::atomic<int> TrackedClass::destructed = 0;

template <typename T>
class HeapPool
{
  public:
    HeapPool(HeapPool &&) = delete;
    HeapPool(const HeapPool &) = delete;
    HeapPool &operator=(HeapPool &&) = delete;
    HeapPool &operator=(const HeapPool &) = delete;

    explicit HeapPool(
        std::size_t initialSize = std::thread::hardware_concurrency()) noexcept
    {
        for (std::size_t i = 0; i < initialSize; ++i)
        {
            doAllocate();
        }
    }

    ~HeapPool() noexcept
    {

        assert(m_size.load(std::memory_order_relaxed) ==
               (std::size_t)(std::distance(m_unusedPool.begin(), m_unusedPool.end()) +
                             std::distance(m_usedPool.begin(), m_usedPool.end())));

        for (auto *obj : m_usedPool)
        {
            delete obj;
        }
        for (auto *obj : m_unusedPool)
        {
            delete obj;
        }
    }

    T *malloc() noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_unusedPool.empty())
            doAllocate();
        T *obj = *m_unusedPool.begin();
        transferHeapOwnershipForNew(obj);
        return obj;
    }

    void free(T *obj) noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            transferHeapOwnershipForDelete(obj);
        }
        checkShrink();
    }

    auto size() const noexcept
    {
        return m_size.load(std::memory_order_relaxed);
    }
    // unusedPool / (unusedPool + usedPool)
    static constexpr auto k_threshold_ratio = 0.75;
    static constexpr auto k_time_out = 2;
    static constexpr auto k_shrink_min_request = 2;

  private:
    std::atomic<std::size_t> m_size;
    std::mutex m_mutex;
    std::atomic<bool> m_shrinking{false};
    std::chrono::steady_clock::time_point m_lastShrinkTime;
    std::unordered_set<T *> m_usedPool;
    std::unordered_set<T *> m_unusedPool;

    void doAllocate() noexcept
    {
        try
        {
            m_unusedPool.insert(new T());
            m_size++;
        }
        catch (const std::bad_alloc &e)
        {
            std::cerr << "new failed: " << e.what() << '\n';
            std::abort();
        }
    }
    void transferHeapOwnershipForDelete(T *obj) noexcept
    {
        m_usedPool.erase(obj);
        m_unusedPool.insert(obj);
    }
    void transferHeapOwnershipForNew(T *obj) noexcept
    {
        m_unusedPool.erase(obj);
        m_usedPool.insert(obj);
    }
    // 当未使用的数量 占总数量 0.75。 unusedPoolSize则缩小一半
    void checkShrink() noexcept
    {
        std::size_t unusedPoolSize =
            std::distance(m_unusedPool.begin(), m_unusedPool.end());
        if (unusedPoolSize < k_shrink_min_request)
            return;

        std::size_t usedPoolSize = std::distance(m_usedPool.begin(), m_usedPool.end());
        double ratio = static_cast<double>(unusedPoolSize) /
                       static_cast<double>(usedPoolSize + unusedPoolSize);

        if (ratio < k_threshold_ratio)
            return;

        if (m_lastShrinkTime.time_since_epoch().count() == 0)
        {
            std::cout << "may trigger shrink because:  (ratio=" << ratio << ")\n";
            m_lastShrinkTime = std::chrono::steady_clock::now();
        }
        else
        {
            // shrink latter for k_time_out
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::steady_clock::now() - m_lastShrinkTime)
                               .count();
            if (elapsed >= k_time_out)
            {
                if (m_shrinking.load(std::memory_order_relaxed))
                    return;
                m_shrinking.store(true, std::memory_order_release);
                shrinkUnusedPool(unusedPoolSize / 2);
                m_lastShrinkTime = {};
                m_shrinking.store(false, std::memory_order_release);
            }
        }
    }

    void shrinkUnusedPool(std::size_t freeSize) noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::size_t free_count = freeSize;
        while (free_count-- > 0)
        {
            T *obj = *m_unusedPool.begin();
            m_unusedPool.erase(obj);
            delete obj;
            m_size--;
        }
        std::cout << "free count: " << freeSize << " , current size: " << m_size << '\n';
    }
};

class MyClass // NOLINT
{

  public:
    MyClass()
    {
        std::cout << "MyClass constructed\n";
    }
    ~MyClass()
    {
        std::cout << "MyClass destroyed\n";
    }
};

void testInitialPoolSize()
{
    TrackedClass::reset();
    {
        HeapPool<TrackedClass> pool(5);
        assert(TrackedClass::constructed == 5);
        assert(TrackedClass::destructed == 0);
    }
    assert(TrackedClass::destructed == 5);
    std::cout << "testInitialPoolSize passed.\n";
}

void testAllocateDeallocate()
{
    TrackedClass::reset();
    {
        HeapPool<TrackedClass> pool(1);
        auto *obj1 = pool.malloc();
        pool.free(obj1);
        auto *obj2 = pool.malloc();
        assert(obj1 == obj2);
        assert(TrackedClass::constructed == 1);
    }
    assert(TrackedClass::destructed == 1);
    std::cout << "testAllocateDeallocate passed.\n";
}

void testAllocateBeyondInitial()
{
    TrackedClass::reset();
    {
        HeapPool<TrackedClass> pool(1);
        auto *obj1 = pool.malloc();
        auto *obj2 = pool.malloc(); // 新建对象
        assert(obj1 != obj2);
        assert(TrackedClass::constructed == 2);
    }
    assert(TrackedClass::destructed == 2);
    std::cout << "testAllocateBeyondInitial passed.\n";
}

void testPartialShrink()
{
    TrackedClass::reset();
    {
        // 未使用为空
        HeapPool<TrackedClass> pool(0);
        std::vector<TrackedClass *> usedObjs;

        // 都会保留 再 已使用集合中
        for (int i = 0; i < 9; ++i) // NOLINT
        {
            usedObjs.push_back(pool.malloc()); // 保留在已使用池
        }

        // 当释放比例  9*0.75 == 6.75 。 至少要 释放7个

        for (int i = 0; i < 7; ++i) // NOLINT
        {
            pool.free(usedObjs[i]);
        }

        // 等待超过收缩时间阈值
        std::this_thread::sleep_for(
            std::chrono::seconds(HeapPool<TrackedClass>::k_time_out));

        // 现在才触发
        auto *temp = pool.malloc();
        pool.free(temp);
        assert(TrackedClass::destructed == 3); // free count: 3
        assert(TrackedClass::constructed == 9);
    }
    assert(TrackedClass::destructed == 9); // 析构时释放所有对象
    std::cout << "testPartialShrink passed.\n";
}

void testThreadSafety()
{
    TrackedClass::reset();
    {
        HeapPool<TrackedClass> pool(10);
        const int num_threads = 4;
        const int num_ops = 1000;
        std::vector<std::thread> threads;

        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([&pool, num_ops]() {
                for (int j = 0; j < num_ops; ++j)
                {
                    auto *obj = pool.malloc();
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    pool.free(obj);
                }
            });
        }

        for (auto &t : threads)
        {
            t.join();
        }
    }
    assert(TrackedClass::constructed == TrackedClass::destructed);
    std::cout << "testThreadSafety passed.\n";
}

int main()
{
    testInitialPoolSize();
    testAllocateDeallocate();
    testAllocateBeyondInitial();
    testPartialShrink();
    testThreadSafety();
    return 0;
}