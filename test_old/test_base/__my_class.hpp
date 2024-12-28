#pragma once

#include <iostream>

template <typename T>
class MyClassT
{
  public:
    explicit MyClassT(T value) : data(value)
    {
        std::cout << "Constructor with value " << data << " at address " << this
                  << std::endl;
    }
    ~MyClassT() = default;

    // 移动构造函数
    MyClassT(MyClassT &&other) noexcept : data(other.data)
    {
        std::cout << "Move Constructor from " << other.data << " at address " << &other
                  << " to " << data << " at address " << this << std::endl;
        other.data = 0; // 将原对象的数据置为默认值
    }

    // 移动赋值运算符
    MyClassT &operator=(MyClassT &&other) noexcept
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
    MyClassT(const MyClassT &other) : data(other.data)
    {
        std::cout << "Copy Constructor from " << other.data << " at address " << &other
                  << " to " << data << " at address " << this << std::endl;
    }

    // 拷贝赋值运算符（用于对比）
    MyClassT &operator=(const MyClassT &other)
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
    void print()
    {
        std::cout << "Data: " << data << " at address " << this << std::endl;
    }

  public:
    T data{}; // NOLINT
};
