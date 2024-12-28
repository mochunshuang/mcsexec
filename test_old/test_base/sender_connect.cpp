#include <concepts>
#include <iostream>
#include <type_traits>
#include <utility>

class A
{
  public:
    // 右值 connect 重载
    template <typename Self, typename Rcvr>
        requires(std::is_rvalue_reference_v<Self &&> ||
                 (std::is_lvalue_reference_v<Self &&> && std::copy_constructible<Self>))
    void connect(this Self && /*self*/,
                 Rcvr /*rcvr*/) // Self && 是转发参数。如果没有requires,左右值都行
    {
        // 这里 self 是右值
        std::cout << "右/左值 connect 调用" << std::endl;
    }

    // 左值 connect 重载
    // template <typename Self, typename Rcvr>
    //     requires(std::copy_constructible<Self>)
    // void connect(this Self &self, Rcvr rcvr) //this Self &self 不是转发参数
    // {
    //     // 这里 self 是左值
    //     std::cout << "左值 connect 调用" << std::endl;
    // }
};

class B
{
  public:
    B() = default;
    // 禁用拷贝构造函数
    B(const B &) = delete;
    B &operator=(const B &) = delete;

    // 右值 connect 重载
    template <typename Self, typename Rcvr>
        requires(std::is_rvalue_reference_v<Self &&> ||
                 (std::is_lvalue_reference_v<Self &&> && std::copy_constructible<Self>))
    void connect(this Self && /*self*/, Rcvr /*rcvr*/)
    {
        std::cout << "右值 connect 调用" << std::endl;
    }
};

int main()
{
    A a;
    A &a_ref = a;

    // 调用左值 connect
    a.connect(a_ref); // 输出: 左值 connect 调用

    // 调用右值 connect
    std::move(a).connect(42); // 输出: 右值 connect 调用

    {
        B b;
        B &b_ref = b;

        // 调用右值 connect
        std::move(b).connect(42); // 输出: 右值 connect 调用

        // 调用左值 connect 将失败，因为 B 不可拷贝构造
        // b.connect(b_ref); // 编译错误
    }

    return 0;
}