#include "../include/execution.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace mcs::execution;
#include <iostream>

#include <iostream>
#include <utility>

struct Tag
{
    void hello() &
    {
        std::cout << "hello() &" << '\n';
    }
    void hello() const &
    {
        std::cout << "hello() const &" << '\n';
    }
    void hello() &&
    {
        std::cout << "hello() &&" << '\n';
    }
};

template <typename T>
void forward_hello(T &&t)
{
    std::forward<T>(t).hello();
}

int main()
{
    Tag tag;
    const Tag const_tag;

    // 左值
    forward_hello(tag);
    // 常量左值
    forward_hello(const_tag);
    // 右值
    forward_hello(Tag{});

    auto &&rv = Tag{};
    static_assert(std::is_same_v<decltype(rv), Tag &&>);
    forward_hello(rv); // 左值。 如何解决
    // 使用 std::move 将 rv 转换为右值
    forward_hello(std::move(rv));

    struct A
    {
        int age; // NOLINT
        A() : age(0)
        {
            std::cout << "A()" << '\n';
        }

        explicit A(int age) : age(age)
        {
            std::cout << " explicit A(int age)" << '\n';
        }

        A(const A &other) : age(other.age)
        {
            std::cout << "A(const A &other)" << '\n';
        }

        A(A &&other) noexcept : age(std::exchange(other.age, 0))
        {
            std::cout << "A(A &&other) noexcept" << '\n';
        }

        A &operator=(const A &other)
        {
            if (this != &other)
            {
                age = other.age;
                std::cout << " A &operator=(const A &other)" << '\n';
            }
            return *this;
        }

        A &operator=(A &&other) noexcept
        {
            if (this != &other)
            {
                age = std::exchange(other.age, 0);
                std::cout << "A &operator=(A &&other)" << '\n';
            }
            return *this;
        }

        ~A() noexcept
        {
            std::cout << "Destructor called" << '\n';
        }
    };

    std::cout << "\nstart: \n";
    {
        auto data = snd::__detail::product_type{A{1}};
        auto &&[a] = data; // 竟然任何操作
        // static_assert(std::is_same_v<decltype(a), A &&>); // 不是的
        static_assert(std::is_same_v<decltype(a), A>);
        int b = a.age;
        std::cout << b << '\n';
    }
    {
        auto data = snd::__detail::product_type{A{1}};
        using T = decltype(data);
        auto &&[a] = static_cast<snd::__detail::product_type<A> &>(data); // 没用
        static_assert(std::is_same_v<decltype(a), A>);

        std::cout << '\n';
        auto &&[b] = static_cast<snd::__detail::product_type<A> &&>(
            data); // A(A &&other) noexcept and Destructor called
        static_assert(std::is_same_v<decltype(b), A>);
    }
    {
        auto data = snd::__detail::product_type{A{1}};

        std::cout << '\n';
        auto &&b = static_cast<snd::__detail::product_type<A> &&>(data).get<0>();
        static_assert(std::is_same_v<decltype(b), A &&>); // A(A &&other) noexcept

        assert(data.get<0>().age == 0); // ok
    }
    {
        auto data = snd::__detail::product_type{A{1}};
        auto &&b = static_cast<std::remove_cvref_t<decltype(data)> &&>(data).get<0>();
        assert(data.get<0>().age == 0); // ok
    }
    // 函数的方式
    {
        auto data = snd::__detail::product_type{A{1}};
        auto fun = []<typename T>(T &&t) {
            auto &&[b] = t;
            assert(t.template get<0>().age == 1);
        };
        fun(data);
        {
            auto fun = []<typename T>(T &&t) {
                auto &&[b] = t;
                assert(t.template get<0>().age ==
                       1); // 没有移动。错误的来源。 引用了不能引用的东西
                return b;
            };
            auto d = fun(std::move(data));
            assert(d.age == 1);
        }
        {
            auto fun = []<typename T>(T &&t) {
                auto &&[b] = t;
                assert(t.template get<0>().age == 1); // 没有移动
                return b;
            };
            auto d = fun(snd::__detail::product_type{A{1}});
            assert(d.age == 1);
        }
        // 看看tuple
        {
            auto fun = []<typename T>(T &&t) {
                auto &&[b] = t;
                assert(std::get<0>(t).age == 1); // 一样没有移动
                return b;
            };
            auto d = fun(std::tuple{A{1}});
            assert(d.age == 1);
        }
    }

    std::cout << "main end\n";
    return 0;
}