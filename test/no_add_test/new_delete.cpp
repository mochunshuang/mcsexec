#include <array>
#include <iostream>
#include <vector>

// NOLINTBEGIN
class MyClass
{
  public:
    MyClass()
    {
        std::cout << "Constructor called\n";
    }
    ~MyClass()
    {
        std::cout << "Destructor called\n";
    }
};

int main()
{
    {
        MyClass *obj = new MyClass(); // 调用构造函数
        delete obj;                   // 调用析构函数并释放内存
    }

    std::cout << "operator new start" << '\n';
    void *block = operator new(sizeof(MyClass)); // 分配原始内存
    std::cout << "operator new end" << '\n';
    MyClass *obj = new (block) MyClass(); // 在原始内存上构造对象
    obj->~MyClass();                      // 手动调用析构函数
    std::cout << "delete(block) start" << '\n';
    operator delete(block); // 释放原始内存
    std::cout << "delete(block) end" << '\n';

    std::cout << "Manager" << '\n';
    {
        struct Manager
        {
            std::array<MyClass, 2> data;
        };
        std::cout << "m start" << '\n';
        Manager m;
        std::cout << "m end" << '\n';
    }
    {
        struct Manager
        {
            std::array<std::aligned_storage_t<sizeof(MyClass), alignof(MyClass)>, 2>
                data; // 未初始化存储
        };
        std::cout << "m start" << '\n';
        Manager m;

        std::cout << "operator new start" << '\n';
        // 手动构造对象
        new (&m.data[0]) MyClass(); // 在第一个位置构造 MyClass
        new (&m.data[1]) MyClass(); // 在第二个位置构造 MyClass
        std::cout << "operator new end" << '\n';

        // 手动调用析构函数
        reinterpret_cast<MyClass *>(&m.data[0])->~MyClass();
        reinterpret_cast<MyClass *>(&m.data[1])->~MyClass();

        std::cout << "m end" << '\n';
    }
    {
        struct Manager
        {
            std::array<std::byte, sizeof(MyClass) * 2> data; // 未初始化存储
        };
        std::cout << "m start" << '\n';
        Manager m;

        std::cout << "operator new start" << '\n';
        // 手动构造对象
        std::construct_at(
            reinterpret_cast<MyClass *>(&m.data[0])); // 在第一个位置构造 MyClass
        std::construct_at(reinterpret_cast<MyClass *>(
            &m.data[sizeof(MyClass)])); // 在第二个位置构造 MyClass
        std::cout << "operator new end" << '\n';

        // 手动调用析构函数
        std::destroy_at(reinterpret_cast<MyClass *>(&m.data[0]));
        std::destroy_at(reinterpret_cast<MyClass *>(&m.data[sizeof(MyClass)]));

        std::cout << "m end" << '\n';
    }
    {
        struct Manager
        {
            struct BlockHeader
            {
                Manager *chunk;
                uint8_t index;
            };

            // 内存块：包含3个(头+对象)的组合
            std::array<std::byte, (sizeof(BlockHeader) + sizeof(MyClass)) * 3> block;

            // 可用索引管理（初始为0,1,2）
            std::vector<uint8_t> available_indices = {0, 1, 2};

            MyClass *allocate()
            {
                if (available_indices.empty())
                    return nullptr;

                // 获取可用索引
                uint8_t index = available_indices.back();
                available_indices.pop_back();

                // 计算块偏移
                size_t offset = index * (sizeof(BlockHeader) + sizeof(MyClass));

                // 设置块头部信息
                BlockHeader *header = reinterpret_cast<BlockHeader *>(&block[offset]);
                header->chunk = this;
                header->index = index;

                // 构造对象
                MyClass *obj =
                    reinterpret_cast<MyClass *>(&block[offset + sizeof(BlockHeader)]);
                std::construct_at(obj);

                return obj;
            }

            static void deallocate(MyClass *ptr)
            {
                if (!ptr)
                    return;

                // 通过对象指针逆向找到块头
                BlockHeader *header = reinterpret_cast<BlockHeader *>(
                    reinterpret_cast<std::byte *>(ptr) - sizeof(BlockHeader));

                // 获取管理器和索引
                Manager *manager = header->chunk;
                uint8_t index = header->index;

                // 析构对象
                std::destroy_at(ptr);

                // 归还索引
                manager->available_indices.push_back(index);
            }
        };

        std::cout << "m start" << '\n';
        Manager m;

        // 测试分配
        MyClass *obj1 = m.allocate();
        MyClass *obj2 = m.allocate();
        MyClass *obj3 = m.allocate();

        // 应该返回nullptr
        MyClass *obj4 = m.allocate();
        std::cout << "Fourth allocation: " << (obj4 ? "success" : "failed") << '\n';

        // 释放测试
        Manager::deallocate(obj2);

        // 再次分配应该复用第二个位置
        MyClass *obj5 = m.allocate();
        std::cout << "Fifth allocation: " << (obj5 ? "success" : "failed") << '\n';

        // 清理所有对象
        Manager::deallocate(obj1);
        Manager::deallocate(obj5);
        Manager::deallocate(obj3);

        std::cout << "m end" << '\n';
    }
    return 0;
}

// NOLINTEND