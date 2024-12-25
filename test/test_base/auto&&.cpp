#pragma once

#include <iostream>
#include <tuple>

struct MyClass
{
    MyClass()
    {
        std::cout << "Default Constructor" << std::endl;
    }
    MyClass(const MyClass & /*unused*/)
    {
        std::cout << "Copy Constructor" << std::endl;
    }
    MyClass(MyClass && /*unused*/) noexcept
    {
        std::cout << "Move Constructor" << std::endl;
    }
    MyClass &operator=(const MyClass &) // NOLINT
    {
        std::cout << "Copy Assignment Operator" << std::endl;
        return *this;
    };
    MyClass &operator=(MyClass && /*unused*/) noexcept
    {
        std::cout << "Move Assignment Operator" << std::endl;
        return *this;
    }
    ~MyClass() = default;
};

void base();
void base_turple();

int main()
{
    // Note: 可以看出 auto && 的变量，只能是引用
    // Note: auto && 可以引用，匿名的，纯右值。延长生命周期？
    base();
    base_turple();
    std::cout << "hello world\n";
    return 0;
}
void base()
{

    std::cout << "base() satrt....\n";
    /**
     * @brief auto && 能bind左值和右值，屈居于等号左边
     *
     */
    MyClass obj;
    // Note: 因为是引用，因此不会发生 移动或复制 操作
    [[maybe_unused]] auto &&r1 = obj;       // r1 是 MyClass&，绑定到左值
    [[maybe_unused]] auto &&r2 = MyClass(); // r2 是 MyClass&&，绑定到右值
    std::cout << "base() end....\n";
    static_assert(std::is_lvalue_reference_v<decltype(r1)>);
    static_assert(std::is_rvalue_reference_v<decltype(r2)>);
}

void base_turple()
{
    std::cout << "\nbase_turple() satrt....\n";
    std::tuple<MyClass> t{MyClass()};
    // Note: 因为是引用，因此不会发生 移动或复制 操作
    // Note: 移动操作，应该发生在 构造 tuple的过程
    // Note: 如果一个类不能 移动，他的turple难以构造。这是以往的经验
    std::cout << "\nauto &&r1 satrt....\n";
    [[maybe_unused]] auto &&r1 = t;
    std::cout << "auto &&r1 end....\n\n";
    [[maybe_unused]] auto &&r2 = std::tuple<MyClass>{MyClass()};
    std::cout << "base_turple() end....\n";
    static_assert(std::is_lvalue_reference_v<decltype(r1)>);
    static_assert(std::is_rvalue_reference_v<decltype(r2)>);
}