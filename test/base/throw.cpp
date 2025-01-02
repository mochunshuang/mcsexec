
#include <cassert>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include "../../include/execution.hpp"

auto test_fun() -> int // NOLINT
{
    throw std::logic_error("error");

    return 1;
}

auto fun1() {}          // NOLINT
auto fun2() noexcept {} // NOLINT

void catch_throw(); // NOLINT
void invokeable();  // NOLINT
void test_make();   // NOLINT
int main()
{
    catch_throw();
    invokeable();
    test_make();

    // 能不能 萃取出 fun 是否抛异常，抛那些异常
    // Note: 不可能。 因此 CS 的签名，必须额外冗余
    // Note 不会有其他信息 可以获取了
    static_assert(not noexcept(test_fun()));
    static_assert(not noexcept(fun1()));
    static_assert(noexcept(fun2()));

    using T = decltype(std::current_exception());
    static_assert(std::is_same_v<std::exception_ptr, T>);
    return 0;
}

void catch_throw()
{
    auto fun = [](std::exception_ptr &&e) {
        // 如何拿到前面的 std::logic_error("error");的信息
        try
        {
            std::rethrow_exception(e); // 重新抛出异常
        }
        catch (const std::logic_error &ex) // 专门捕获 std::logic_error
        {
            assert(std::string(ex.what()) == "error");
        }
        catch (const std::exception &ex) // 捕获其他 std::exception 派生类
        {
            assert(false);
        }
        catch (...) // 捕获所有其他异常
        {
            assert(false);
        }
        // Note: 返回值只能是一个类型 。 std::exception_ptr 是很好的抽象
    };
    try
    {
        [[maybe_unused]] int ret = test_fun();
        assert(false);
    }
    catch (...)
    {
        fun(std::current_exception());
    }
}
void invokeable() // NOLINT
{
    auto fun0 = [](std::exception_ptr e) {
        try
        {
            std::rethrow_exception(e); // 重新抛出异常
        }
        catch (const std::logic_error &ex) // 专门捕获 std::logic_error
        {
            assert(std::string(ex.what()) == "error");
        }
        catch (const std::exception &ex) // 捕获其他 std::exception 派生类
        {
            assert(false);
        }
        catch (...) // 捕获所有其他异常
        {
            assert(false);
        }
    };

    auto fun1 = [](std::exception_ptr &&e) {
        try
        {
            std::rethrow_exception(e); // 重新抛出异常
        }
        catch (const std::logic_error &ex) // 专门捕获 std::logic_error
        {
            assert(std::string(ex.what()) == "error");
        }
        catch (const std::exception &ex) // 捕获其他 std::exception 派生类
        {
            assert(false);
        }
        catch (...) // 捕获所有其他异常
        {
            assert(false);
        }
    };

    auto fun2 = [](const std::exception_ptr &e) {
        try
        {
            std::rethrow_exception(e); // 重新抛出异常
        }
        catch (const std::logic_error &ex) // 专门捕获 std::logic_error
        {
            assert(std::string(ex.what()) == "error");
        }
        catch (const std::exception &ex) // 捕获其他 std::exception 派生类
        {
            assert(false);
        }
        catch (...) // 捕获所有其他异常
        {
            assert(false);
        }
    };

    auto fun3 = [](std::exception_ptr &e) {
        try
        {
            std::rethrow_exception(e); // 重新抛出异常
        }
        catch (const std::logic_error &ex) // 专门捕获 std::logic_error
        {
            assert(std::string(ex.what()) == "error");
        }
        catch (const std::exception &ex) // 捕获其他 std::exception 派生类
        {
            assert(false);
        }
        catch (...) // 捕获所有其他异常
        {
            assert(false);
        }
    };

    // fun0
    {
        try
        {
            [[maybe_unused]] int ret = test_fun();
            assert(false);
        }
        catch (...)
        {
            fun0(std::current_exception());
        }
    }
    // fun1
    {
        try
        {
            [[maybe_unused]] int ret = test_fun();
            assert(false);
        }
        catch (...)
        {
            fun1(std::current_exception());
        }
    }
    // fun2
    {
        try
        {
            [[maybe_unused]] int ret = test_fun();
            assert(false);
        }
        catch (...)
        {
            fun2(std::current_exception());
        }
    }

    // fun3
    {
        try
        {
            [[maybe_unused]] int ret = test_fun();
            assert(false);
        }
        catch (...)
        {
            // fun3(std::current_exception()); // 编译器错误
        }
    }
    // 测试
    using F0 = decltype(fun0);
    using F1 = decltype(fun1);
    using F2 = decltype(fun2);
    using F3 = decltype(fun3);
    using T = decltype(std::current_exception());

    // static_assert(std::is_invocable_v<F0, T>); // 失败为何?
    using namespace mcs::execution; // NOLINT
    static_assert(functional::callable<F0, T>);
    static_assert(functional::callable<F1, T>);
    static_assert(functional::callable<F2, T>);
    static_assert(not functional::callable<F3, T>); // 满足
}
void test_make()
{
    std::exception_ptr eptr = std::make_exception_ptr(1); // 创建一个 int 类型的异常

    try
    {
        std::rethrow_exception(eptr); // 重新抛出异常
    }
    catch (int value)
    { // 直接捕获 int 类型的异常
        std::cout << "Caught an int exception with value: " << value << "\n";
    }
    catch (...)
    { // 捕获其他类型的异常
        std::cout << "Caught an unknown exception!\n";
    }
}