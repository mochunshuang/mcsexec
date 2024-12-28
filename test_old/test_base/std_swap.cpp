#include <atomic>
#include <iostream>
#include <utility> // 包含 std::swap

struct Base
{

    virtual ~Base() = delete;
    virtual void notify() = 0;

    Base() = default;
    Base(Base &&) = delete;
    Base(const Base &) = delete;
    Base &operator=(Base &&) = delete;
    Base &operator=(const Base &) = delete;

    std::atomic<Base *> next{nullptr}; // NOLINT
};

struct Base_class : virtual Base
{
    void notify() override
    {
        //
    }
};

// 自定义空类
class Tag
{
};
struct Data
{
    // 填充属性，让其变得复杂
    int value;
    std::string name;
    Base_class b;
};

struct MyClass
{
    Tag poniter;
    Data *data;
};

void swap_full_class();

void swap_pointer();
void swap_empty_class();

int main()
{
    swap_pointer();
    swap_empty_class();
    return 0;
}

void swap_full_class()
{
    /**
     * @brief 简单的 空对象，指针，作为成员数据。不会有问题
     *
     */
    MyClass a{};
    MyClass b{};
    std::swap(a, b);
    {
        auto *a = new MyClass;
        auto *b = new MyClass;
        std::cout << "Before swap: a = " << &a << ", b = " << &b << std::endl;
        std::swap(a, b);
        std::cout << "After swap: a = " << &a << ", b = " << &b << std::endl;
        delete a;
        delete b;
    }
}

void swap_empty_class()
{
    Tag a, b; // NOLINT
    std::cout << "Before swap: a = " << &a << ", b = " << &b << std::endl;

    std::swap(a, b); // 使用 std::swap 交换

    std::cout << "After swap: a = " << &a << ", b = " << &b << std::endl;
}

void swap_pointer()
{
    int x = 10, y = 20;         // NOLINT
    int *ptr1 = &x, *ptr2 = &y; // NOLINT

    std::cout << "Before swap: ptr1 = " << ptr1 << " (" << *ptr1 << "), ptr2 = " << ptr2
              << " (" << *ptr2 << ")" << std::endl;

    std::swap(ptr1, ptr2); // 使用 std::swap 交换指针

    std::cout << "After swap: ptr1 = " << ptr1 << " (" << *ptr1 << "), ptr2 = " << ptr2
              << " (" << *ptr2 << ")" << std::endl;
}