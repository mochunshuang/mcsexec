#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstdint>
#include <forward_list>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

class MyClass // NOLINT
{
  public:
    int value; // NOLINT

    MyClass() noexcept : value(0) {};
    MyClass(int v) : value(v)
    {
        std::cout << "构造函数调用, value = " << value << std::endl; // NOLINT
    }

    ~MyClass()
    {
        std::cout << "析构函数调用, value = " << value << std::endl; // NOLINT
    }
};

template <typename T>
class ResourcePool
{
    std::forward_list<T *> m_resources;

    T *get_form_recycle() noexcept // NOLINT
    {
        if (m_resources.empty())
            return nullptr;
        T *resource = m_resources.front();
        m_resources.pop_front();
        return resource;
    }

  public:
    T *header()
    {
        auto *obj = get_form_recycle();

        if (obj != nullptr)
            return obj;

        return new T();
    }
    void append_front(T *id) noexcept
    {
        m_resources.push_front(id);
    }
    ResourcePool(const ResourcePool &) = delete;
    ResourcePool &operator=(const ResourcePool &) = delete;

    ResourcePool(ResourcePool &&) = default;
    ResourcePool &operator=(ResourcePool &&) = default;

    ResourcePool() = default;
    ~ResourcePool() noexcept
    {
        for (auto &v : m_resources)
        {
            delete v;
        }
    }
};

int main()
{
    std::list<int *> heads;
    assert(heads.size() == 0);

    std::vector<int *> v;
    assert(v.size() == 0);

    {
        std::array<int *, 6> v{};
        assert(v.size() == 6);
        assert(v[0] == nullptr);
    }

    {
        std::forward_list<int *> bucket;
        assert(bucket.empty());
    }

    {
        std::forward_list<MyClass> bucket;
        assert(bucket.empty());
        bucket.emplace_front(1); // 只要一次构造函数调用，不调用移动构造
        assert(not bucket.empty());
    }
    {
        class Class // NOLINT
        {
          public:
            Class()
            {
                std::cout << "构造函数调用 \n"; // NOLINT
            }

            ~Class()
            {
                std::cout << "析构函数调用\n"; // NOLINT
            }
        };
        std::forward_list<Class> bucket;
        assert(bucket.empty());
        bucket.emplace_front(); // 只要一次构造函数调用，不调用移动构造
        assert(not bucket.empty());
    }

    std::cout << "std::set: \n";
    {
        auto compare = [](const MyClass &a, const MyClass &b) {
            return a.value < b.value;
        };
        std::set<MyClass, decltype(compare)> bucket;
        assert(bucket.empty());
        bucket.emplace(1);
        assert(not bucket.empty());

        bucket.emplace(5); // NOLINT
        bucket.emplace(2);
        bucket.emplace(4);
        bucket.emplace(3);
        for (const auto &obj : bucket)
        {
            std::cout << obj.value << " "; // 输出: 1 2 3 4 5
        }
        assert(bucket.size() == 5);
        std::cout << '\n';

        // 保存某个元素的迭代器和引用
        auto it = bucket.find(MyClass(3)); // 找到值为 3 的元素的迭代器
        assert(it != bucket.end());        // 确保找到
        const MyClass &ref = *it;          // 保存引用
        // 验证迭代器和引用是否仍然有效
        assert(it->value == 3); // 迭代器仍然有效
        assert(ref.value == 3); // 引用仍然有效
                                //
        // 使用 erase_if 删除满足条件的元素
        std::erase_if(bucket, [](const MyClass &obj) {
            return obj.value % 2 == 0; // 删除值为偶数的元素
        });
        for (const auto &obj : bucket)
        {
            std::cout << obj.value << " "; // 输出: 1  3  5
        }
        std::cout << '\n';

        // 验证迭代器和引用是否仍然有效
        assert(it->value == 3); // 迭代器仍然有效
        assert(ref.value == 3); // 引用仍然有效
    }

    {
        struct A
        {
            std::array<int, 3> bucket;
        };

        // 不是每个int* 都插入 A*， 就第一次添加。减少空间
        std::unordered_map<int *, A *> map; // 说明 A 能操作，不能说明什么
        assert(map.size() == 0);

        auto *obj = new A{};

        // 为了 找到 A*
        map.insert({obj->bucket.data(), obj});

        //

        assert(map.size() == 1);
        std::cout << "max_size" << map.max_size() << "\n";

        // 如何快速的 确定 状态。 key + status。 释放的时候，可以重复使用
        //  A* 本身就是能够确定状态的

        delete obj;
    }
    {
        ResourcePool<int> pool;

        int *resource1 = pool.header();
        std::cout << "Resource 1: " << *resource1 << '\n';

        pool.append_front(resource1);

        int *resource2 = pool.header();

        assert(resource1 == resource2);

        std::cout << "Resource 2: " << *resource2 << '\n';
    }
    {
        ResourcePool<MyClass> pool;

        auto *resource1 = pool.header();
        std::cout << "Resource 1: " << resource1 << '\n';

        pool.append_front(resource1);

        auto *resource2 = pool.header();

        assert(resource1 == resource2);

        auto *obj = new (resource1) MyClass(2);
        assert(resource1 == obj);
        pool.append_front(obj);
    }
    {
        struct A
        {
            int &ref; // NOLINT

            explicit A(int &r) : ref(r) {}
        };
        int a = 2;
        A obj(a);
        assert(obj.ref == a);
        assert(&(obj.ref) == &a);

        {
            int *p = &a;
            A o(*p); // 指针，还需要*
            assert(&(o.ref) == &a);
        }
    }
    {
        int8_t v = 0b00000010;
        // 设计一个方法, 从 左往右，索引从0 开始算
        int index = 5; // NOLINT
        // 让 v  的 5索引置为 1
        constexpr auto end_index = CHAR_BIT - 1; // NOLINT
        v |= (1 << (end_index - index));         // NOLINT

        assert(v == 0b00000110);

        // 反过来，让 第 5  位置 为0，其他不变呢
        v &= ~(1 << (end_index - index));
        assert(v == 0b00000010);
        // 检查 从左往右第6 位一定是
        index = 6;
        std::cout << "v: " << std::bitset<8>(v) << ", index: " << index << std::endl;
        assert((v & (1 << (7 - index))) != 0 && "The specified bit in status must be 1");
        assert((v & (1 << (7 - 0))) == 0 && "The specified bit in status must be 0");

        std::cout << "v & (1 << (7 - index): " << std::bitset<8>((v & (1 << (7 - index))))
                  << ", index: " << index << std::endl;
        std::cout << "v & (1 << (7 - 0): " << std::bitset<8>((v & (1 << (7 - 0))))
                  << ", index: " << index << std::endl;

        std::array<int, 1> data{};
        int *p = &data[0]; // NOLINT
        assert(*p == 0);
        {
            using status_type = unsigned long long; // NOLINT

            static constexpr auto MAX_NUM =              // NOLINT
                std::numeric_limits<status_type>::max(); // NOLINT
            static_assert(std::countl_zero(MAX_NUM) == 0);
            static_assert(std::popcount(MAX_NUM) == 64); // NOLINT
        }
    }
    {
        struct A
        {
            bool equel(volatile A *p) volatile // 添加 volatile 修饰
            {
                return p == this;
            }
            bool equel2(A *p)
            {
                return p == this;
            }
        };

        for (int i = 0; i < 10000; ++i) // NOLINT
        {
            volatile A *p = new A;
            assert(p->equel(p)); // 现在可以正常调用
            delete p;
        }

        for (int i = 0; i < 10000; ++i) // NOLINT
        {
            A *p = new A;
            assert(p->equel2(p)); // 现在可以正常调用
            delete p;
        }
    }

    std::cout << "main done\n";
    return 0;
}