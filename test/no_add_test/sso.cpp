#include <memory>
#include <iostream>
#include <stdexcept>

// NOLINTBEGIN
// 定义函数指针类型
using PrintFuncPtr = void (*)(void *);
using CalculateFuncPtr = int (*)(void *, int);

// 类型擦除包装器
class AnyObject
{
  public:
    // 模板构造函数，接受任何类型的 shared_ptr
    template <typename T>
    AnyObject(std::shared_ptr<T> obj)
        : object_(obj), print_func_(&AnyObject::callPrint<T>),
          calculate_func_(&AnyObject::callCalculate<T>)
    {
    }

    // 检查并调用 print 方法
    void print()
    {
        if (!print_func_)
        {
            throw std::runtime_error("Object does not support print()");
        }
        print_func_(object_.get());
    }

    // 检查并调用 calculate 方法
    int calculate(int x)
    {
        if (!calculate_func_)
        {
            throw std::runtime_error("Object does not support calculate(int)");
        }
        return calculate_func_(object_.get(), x);
    }

    // 恢复原始类型的 shared_ptr
    template <typename T>
    std::shared_ptr<T> as() const
    {
        return std::static_pointer_cast<T>(object_);
    }

  private:
    // 静态成员函数模板，用于调用特定类型的成员函数
    template <typename T>
    static void callPrint(void *objPtr)
    {
        static_cast<T *>(objPtr)->print();
    }

    template <typename T>
    static int callCalculate(void *objPtr, int x)
    {
        return static_cast<T *>(objPtr)->calculate(x);
    }

    std::shared_ptr<void> object_;
    PrintFuncPtr print_func_ = nullptr;
    CalculateFuncPtr calculate_func_ = nullptr;
};

// 示例类
class MyClass1
{
  public:
    void print()
    {
        std::cout << "MyClass1::print() called" << std::endl;
    }

    int calculate(int x)
    {
        return x * 2;
    }
};

int main()
{
    constexpr auto size = sizeof(std::shared_ptr<void>); // 16

    auto obj = std::make_shared<MyClass1>();
    AnyObject any(obj);

    any.print();
    std::cout << "Result: " << any.calculate(5) << std::endl;

    return 0;
}
// NOLINTEND