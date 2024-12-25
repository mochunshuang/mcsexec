#include <cassert>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <utility>

struct MySender
{
    // 假设 MySender 是一个 basic_sender
};

struct MyReceiver
{
    // 假设 MyReceiver 是一个 receiver
};

template <typename Self, typename Rcvr>
    requires(std::is_rvalue_reference_v<Self &&>)
auto connect(Self &&self, Rcvr rcvr)
{
    static_assert(not std::is_lvalue_reference_v<decltype(self)>);
    static_assert(std::is_lvalue_reference_v<decltype((self))>);
    std::cout << "is_rvalue_reference_v \n";
}

template <typename Self, typename Rcvr>
    requires(std::is_lvalue_reference_v<Self &&> && std::copy_constructible<Self>)
auto connect(Self &&self, Rcvr rcvr)
{
    static_assert(std::is_lvalue_reference_v<decltype(self)>);
    static_assert(std::is_lvalue_reference_v<decltype((self))>);
    std::cout << "is_lvalue_reference_v \n";
}

template <typename Self, typename Rcvr>

auto connect2(Self &&self, Rcvr rcvr)
    requires(std::is_rvalue_reference_v<decltype(self)>)
{
    std::cout << "is_rvalue_reference_v \n";
}

template <typename Self, typename Rcvr>
auto connect2(Self &&self, Rcvr rcvr)
    requires(std::is_lvalue_reference_v<decltype(self)> &&
             std::copy_constructible<decltype(self)>)
{

    std::cout << "is_lvalue_reference_v \n";
}

int main()
{
    MySender sender;
    MyReceiver receiver;
    // 右值引用版本
    connect(std::move(sender), receiver);
    // 左值引用版本
    connect(sender, receiver);
    {
        // Note: 和上面是等价
        connect2(std::move(sender), receiver);
        connect2(sender, receiver);
    }
    {
        int a = 1;
        [[maybe_unused]] int b = std::move(a); // 没有用
        assert(a == 1);
    }
    {
        struct A
        {
            int a;
        };
        A a = A(1);
        [[maybe_unused]] auto b = std::move(a); // 没有用
        assert(a.a == 1);
    }
    {
        struct B
        {
            int a;
            B(int v) noexcept : a(v) {}
            B(B &&b) noexcept : a(b.a)
            {
                b.a = 0;
            }
        };
        B a = B(1);
        [[maybe_unused]] auto b = std::move(a); // 自定义的移动构造才有用
        assert(a.a == 0);
        // Note: 优先默认的构造，0原则
        {
            auto fun = []<typename T>(T &&t) {
                auto &&[b] = t;
                assert(std::get<0>(t).a == 0); // 已经移动了
                return std::move(b);
            };
            auto d = fun(std::tuple{B{1}});
            assert(d.a == 0);
        }
    }

    return 0;
}