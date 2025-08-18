#include <iostream>
#include <stdexcept>

// NOLINTBEGIN

// 声明为noexcept的函数
void func() noexcept
{
    throw std::runtime_error("Exception from noexcept function");
}

void test() noexcept
{
    try
    {
        func();
    }
    catch (const std::exception &e)
    {
        std::cout << ">>> Caught exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << ">>> Caught unknown exception" << std::endl;
    }
}

void func2()
{
    throw std::runtime_error("Exception from noexcept function");
}
void test2() noexcept
{
    func2();
}

void test3() noexcept
{
    try
    {
        test2();
    }
    catch (const std::exception &e)
    {
        std::cout << ">>> Caught exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << ">>> Caught unknown exception" << std::endl;
    }
}

int main()
{

    /*
// NOTE: noexcept 说明不再捕获
terminate called after throwing an instance of 'std::runtime_error'
what():  Exception from noexcept function
*/
#if 0
    test();
#endif
    test3();
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND