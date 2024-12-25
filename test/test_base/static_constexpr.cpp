#include <iostream>

class MyClass
{
    // static constexpr不会占用类的实例空间，因为它们是静态的，属于类本身，而不是类的实例
  public:
    static constexpr int value = 42; // 静态常量成员

    void printValue() const
    {
        std::cout << "Value: " << value << std::endl;
    }
};

int main()
{
    MyClass obj1;
    MyClass obj2;

    obj1.printValue(); // 输出: Value: 42
    obj2.printValue(); // 输出: Value: 42

    // 1: 空类优化
    std::cout << "Size of MyClass: " << sizeof(MyClass) << std::endl;

    return 0;
}