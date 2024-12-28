#include <iostream>
#include <vector>
#include <utility> // for std::move

class MyClass
{
  public:
    MyClass(int value = 0) : data(value)
    {
        std::cout << "Constructor with value " << data << " at address " << this
                  << std::endl;
    }

    // 移动构造函数
    MyClass(MyClass &&other) noexcept : data(other.data)
    {
        std::cout << "Move Constructor from " << other.data << " at address " << &other
                  << " to " << data << " at address " << this << std::endl;
        other.data = 0; // 将原对象的数据置为默认值
    }

    // 移动赋值运算符
    MyClass &operator=(MyClass &&other) noexcept
    {
        if (this != &other)
        {
            data = other.data;
            std::cout << "Move Assignment Operator from " << other.data << " at address "
                      << &other << " to " << data << " at address " << this << std::endl;
            other.data = 0; // 将原对象的数据置为默认值
        }
        return *this;
    }

    // 拷贝构造函数（用于对比）
    MyClass(const MyClass &other) : data(other.data)
    {
        std::cout << "Copy Constructor from " << other.data << " at address " << &other
                  << " to " << data << " at address " << this << std::endl;
    }

    // 拷贝赋值运算符（用于对比）
    MyClass &operator=(const MyClass &other)
    {
        if (this != &other)
        {
            data = other.data;
            std::cout << "Copy Assignment Operator from " << other.data << " at address "
                      << &other << " to " << data << " at address " << this << std::endl;
        }
        return *this;
    }

    // 打印数据和地址
    void print() const
    {
        std::cout << "Data: " << data << " at address " << this << std::endl;
    }

  private:
    int data;
};

void base_test();
int main()
{
    base_test();

    return 0;
}
void base_test()
{
    // 创建一个包含 3 个 MyClass 对象的 std::vector
    std::vector<MyClass> vec1;
    vec1.emplace_back(1);
    vec1.emplace_back(2);
    vec1.emplace_back(3);

    std::cout << "\n--- Moving vec1 to vec2 ---" << std::endl;
    std::vector<MyClass> vec2 = std::move(vec1);

    std::cout << "\n--- Printing vec1 ---" << std::endl;
    for (const auto &obj : vec1)
    {
        obj.print();
    }

    std::cout << "\n--- Printing vec2 ---" << std::endl;
    for (const auto &obj : vec2)
    {
        obj.print();
    }

    std::cout << "\n--- Moving vec2 to vec3 ---" << std::endl;
    std::vector<MyClass> vec3;
    vec3 = std::move(vec2);

    std::cout << "\n--- Printing vec2 ---" << std::endl;
    for (const auto &obj : vec2)
    {
        obj.print();
    }

    std::cout << "\n--- Printing vec3 ---" << std::endl;
    for (const auto &obj : vec3)
    {
        obj.print();
    }
}