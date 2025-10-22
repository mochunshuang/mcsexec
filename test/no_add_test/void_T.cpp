#include <bit>
#include <iostream>

// NOLINTBEGIN

// 定义一组操作函数类型
using ConstructFunc = void (*)(void *buffer);
using DestructFunc = void (*)(void *obj);
using CopyFunc = void (*)(void *dest, const void *src);
using MoveFunc = void (*)(void *dest, void *src);
using PrintFunc = void (*)(const void *obj);

// 具体类型的操作函数实现
template <typename T>
void construct_impl(void *buffer)
{
    new (buffer) T(); // 默认构造
}

template <typename T>
void destruct_impl(void *obj)
{
    static_cast<T *>(obj)->~T(); // 析构
}

template <typename T>
void copy_impl(void *dest, const void *src)
{
    new (dest) T(*static_cast<const T *>(src)); // 拷贝构造
}

template <typename T>
void move_impl(void *dest, void *src)
{
    new (dest) T(std::move(*static_cast<T *>(src))); // 移动构造
    static_cast<T *>(src)->~T();                     // 析构源对象
}

template <typename T>
void print_impl(const void *obj)
{
    std::cout << *static_cast<const T *>(obj) << std::endl;
}

// 测试类
class MyClass
{
  public:
    int value;

    MyClass(int v = 0) : value(v)
    {
        std::cout << "构造 MyClass(" << value << ")\n";
    }

    MyClass(const MyClass &other) : value(other.value)
    {
        std::cout << "拷贝构造 MyClass(" << value << ")\n";
    }

    MyClass(MyClass &&other) noexcept : value(other.value)
    {
        other.value = 0;
        std::cout << "移动构造 MyClass(" << value << ")\n";
    }

    ~MyClass()
    {
        std::cout << "析构 MyClass(" << value << ")\n";
    }

    friend std::ostream &operator<<(std::ostream &os, const MyClass &obj)
    {
        return os << "MyClass{" << obj.value << "}";
    }
};

int main()
{
    // 分配内存缓冲区
    alignas(MyClass) char buffer1[sizeof(MyClass)];
    alignas(MyClass) char buffer2[sizeof(MyClass)];
    alignas(MyClass) char buffer3[sizeof(MyClass)];

    // 直接使用函数指针 + void* 操作
    std::cout << "=== 1. 在buffer1中构造对象 ===" << std::endl;
    construct_impl<MyClass>(buffer1); // void* + 函数

    // 访问对象并设置值
    std::bit_cast<MyClass *>(static_cast<void *>(buffer1))->value = 100;

    std::cout << "\n=== 2. 在buffer2中拷贝buffer1 ===" << std::endl;
    copy_impl<MyClass>(buffer2, buffer1); // void* + 函数

    std::cout << "\n=== 3. 打印对象 ===" << std::endl;
    print_impl<MyClass>(buffer1);
    print_impl<MyClass>(buffer2);

    std::cout << "\n=== 4. 移动buffer2到buffer3 ===" << std::endl;
    move_impl<MyClass>(buffer3, buffer2);

    std::cout << "\n=== 5. 打印移动后的对象 ===" << std::endl;
    print_impl<MyClass>(buffer3);

    std::cout << "\n=== 6. 析构所有对象 ===" << std::endl;
    destruct_impl<MyClass>(buffer1);
    destruct_impl<MyClass>(buffer3);

    return 0;
}
// NOLINTEND