#include <iostream>
#include <type_traits>

// 测试类 A
template <typename Sndr>
class A
{
  public:
    // 构造函数，接受转发引用
    A(Sndr &&sndr) // Note: 这里是,右值类型 // NOLINT
    {
        if (std::is_lvalue_reference_v<Sndr>)
        {
            std::cout << "Sndr is an lvalue reference: Sndr&" << '\n';
        }
        else
        {
            std::cout << "Sndr is an rvalue reference: Sndr&&" << '\n';
        }
    }
};

// 测试函数
void test() // NOLINT
{
    int x = 10; // NOLINT

    // A a1{x};  //  编译异常
    // A<int> a1{x}; //  编译异常
    A a2{10}; // 10 是右值 // NOLINT
}

int main()
{
    test();
    return 0;
}