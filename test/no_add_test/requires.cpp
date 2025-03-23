
// NOLINTBEGIN

#include <concepts>
#include <iostream>
#include <type_traits>

#include "./completion_signatures.h"

static_assert(valid_completion_signatures<completion_signatures<>>);

template <typename T>
auto get()
{
    if constexpr (std::is_same_v<T, int>)
        return 1;
    else
        return completion_signatures<>{};
}

template <typename T>
struct template_test;

template <typename T>
struct template_test
{
    using type = T;
};

template <>
struct template_test<void>
{
};

template <typename T>
using get_template_test_t = typename template_test<T>::type;

// NOTE: -> std::integral 是约束返回值
template <typename T>
    requires(requires {
        { get<T>() } -> valid_completion_signatures;
        // { get_template_test_t<T> } -> std::integral; // NOTE: 别名不是类型
        { auto(get_template_test_t<T>{}) } -> std::integral;
    })
void test() {};

template <typename T>
// requires(requires { not std::same_as<int, T>; })  // 错误，不是约束T不是int
    requires(requires {
        requires !std::same_as<T, int>; // 在 requires 表达式中使用 requires
    })
void test2()
{
}

template <typename T>
consteval static auto test_cs()
{
    if (requires { typename T::type; })
    {
    }
    else
    {
        throw;
    }
}

template <typename T>
// requires requires { using ValueType = T; } //失败
    requires requires { test_cs<T>(); }
void test3()
{
}

int main()
{
    static_assert(std::integral<bool>);
    // test<int>(); // 失败
    test<bool>(); // 成功

    {
        // test2<int>();//失败
        test2<bool>(); // OK
    }

    {
        test3<template_test<void>>();
    }
    std::cout << "main done\n";
    return 0;
}

// NOLINTEND