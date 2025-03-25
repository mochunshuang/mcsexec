#include <cassert>
#include <cstdlib>
#include <exception>
#include <source_location>
#include <stdexcept>
#include <iostream>

// NOLINTBEGIN
enum State
{
    joined,
    unused,
    unused_and_closed,
    other
};

void simple_counting_scope(State state) noexcept
{
    // If state is not one of joined, unused, or unused-and-closed, terminate with a
    // custom message.
    if (state != joined && state != unused && state != unused_and_closed)
        throw std::runtime_error(
            "Invalid state: State must be one of joined, unused, or unused-and-closed.");
}

void test()
{
    std::source_location loc = std::source_location::current();
    std::cerr
        << "Error at " << loc.file_name() << ":" << loc.line() << " in function "
        << loc.function_name() << "\n"
        << "Invalid state: State must be one of joined, unused, or unused-and-closed.\n";
    std::terminate();
}

void test2()
{
    std::abort();
}
void test3()
{
    assert(false);
}
int main()
{

    // NOTE: 测试外面还能 catch 吗。答案是不能。 因此： throw + noexcept == std::terminate
    // NOTE: 但是带有 自定义的信息:  what():  Invalid state:xxxxx
    // try
    // {
    //     simple_counting_scope(other); // 这里传入一个无效的状态
    // }
    // catch (const std::exception &e)
    // {
    //     std::cerr << "Error: " << e.what() << std::endl;
    // }

    // test(); // [ terminate called without an active exception ]
    // try
    // {
    //     test();
    // }
    // catch (...)
    // {
    //     // NOTE: 失败,无法catch
    //     std::cout << " catch std::terminate \n";
    // }

    // test2(); // 一点信息都没有更不好

    // test3();  // assert 带源码信息
    return 0;
}
// NOLINTEND