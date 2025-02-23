#include <iostream>
#include <array>
#include <string>
#include <cassert>

// NOLINTBEGIN
template <typename T>
struct Chunk
{
    struct BlockHeader
    {
        Chunk *chunk;
        uint8_t index;
    };

    struct Block
    {
        alignas(alignof(BlockHeader)) std::byte data[sizeof(BlockHeader) + sizeof(T)];
    };
    std::array<Block, 2> blocks; // 没有分配内存哦
};

struct MyClass
{
    std::string name;

    MyClass() : name("Default Name")
    {
        std::cout << "MyClass constructed: " << name << std::endl;
    }

    ~MyClass()
    {
        std::cout << "MyClass destroyed: " << name << std::endl;
    }
};

int main()
{
    Chunk<MyClass> c;
    [[maybe_unused]] auto &v = c.blocks[0];

    // 强制转化
    MyClass *ptr = nullptr;
    {
        using T = MyClass;
        auto &blocks = c.blocks;
        int index = 0;

        // 获取 BlockHeader 指针
        auto *header =
            reinterpret_cast<Chunk<T>::BlockHeader *>(blocks[index].data); // NOLINT

        // 设置 BlockHeader 的值
        header->chunk = &c;
        header->index = index;

        // 验证 BlockHeader 的值是否正确
        std::cout << "BlockHeader: chunk = " << header->chunk
                  << ", index = " << static_cast<int>(header->index) << '\n';

        // 计算 MyClass 对象的指针
        ptr = reinterpret_cast<T *>(blocks[index].data + sizeof(Chunk<T>::BlockHeader));

        // 验证指针是否对齐
        assert(reinterpret_cast<uintptr_t>(ptr) % alignof(T) == 0);

        // 在 ptr 的位置构造 MyClass 对象
        // std::cout << "MyClass name: " << ptr->name << '\n'; //Note: 乱码，未初始化
        new (ptr) MyClass();
        ptr->name = "New Name";

        // 验证 MyClass 对象的值
        assert(ptr->name == "New Name");

        // 从 ptr 反推出 BlockHeader* 拿到上面的值，能否做到？
        // 也就是从 MyClass* 算出 附加的 BlockHeader* 信息，当然  MyClass* 必须是
        // blocks分配的
        auto *headerFromPtr = reinterpret_cast<Chunk<T>::BlockHeader *>(
            reinterpret_cast<std::byte *>(ptr) - sizeof(Chunk<T>::BlockHeader));

        // 验证反推的 BlockHeader 是否正确
        std::cout << "BlockHeader from ptr: chunk = " << headerFromPtr->chunk
                  << ", index = " << static_cast<int>(headerFromPtr->index) << '\n';

        // 任意设置都是有效的。指针的值是相同的
        header->index = index + 1;
        assert(headerFromPtr->index == (index + 1));
        assert(headerFromPtr == header);
        // Note: 起始地址相同的话
        auto &v = c.blocks[0];
        // Note: 就是这么奇妙
        assert((long *)&v == (long *)header);
        std::cout << &v << " == " << header << "\n";
    }
    std::cout << "\n";
    // 手动调用析构函数
    ptr->~MyClass();

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND