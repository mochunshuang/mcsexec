#include <iostream>

class MyClass
{
  public:
    MyClass() = default;

    MyClass(const MyClass & /*other*/)
    {
        std::cout << "MyClass 拷贝构造函数被调用\n";
    }
    // NOLINTNEXTLINE
    MyClass &operator=(const MyClass & /*other*/)
    {

        std::cout << "MyClass 赋值运算符被调用\n";
        return *this;
    }
    MyClass(MyClass &&) = delete;
    MyClass &operator=(const MyClass &&) = delete;

    ~MyClass() = default;
};

int main()
{

    /**
     * @brief 证明了。对象指针赋值，不会调用构造函数
     *
     */
    auto *ptr1 = new MyClass;
    std::cout << "指针赋值前\n";
    // 指针赋值运算
    [[maybe_unused]] auto *p = ptr1;
    std::cout << "指针赋值后\n";

    return 0;
}