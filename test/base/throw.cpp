
#include <cassert>
#include <exception>
#include <stdexcept>
#include <type_traits>

auto fun0() -> int // NOLINT
{
    throw std::logic_error("error");

    return 1;
}

auto fun1() {}          // NOLINT
auto fun2() noexcept {} // NOLINT

auto catch_throw(); // NOLINT
int main()
{

    // 能不能 萃取出 fun 是否抛异常，抛那些异常
    // Note: 不可能。 因此 CS 的签名，必须额外冗余
    // Note 不会有其他信息 可以获取了
    static_assert(not noexcept(fun0()));
    static_assert(not noexcept(fun1()));
    static_assert(noexcept(fun2()));

    using T = decltype(std::current_exception());
    static_assert(std::is_same_v<std::exception_ptr, T>);
    return 0;
}

auto catch_throw()
{
    auto fun = [](std::exception_ptr &&e) {
        // 如何拿到前面的 std::logic_error("error");的信息
        try
        {
            std::rethrow_exception(e); // 重新抛出异常
        }
        catch (const std::logic_error &ex) // 专门捕获 std::logic_error
        {
            assert(std::string(ex.what()) == "err");
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
        [[maybe_unused]] int ret = fun0();
        assert(false);
    }
    catch (...)
    {
        fun(std::current_exception());
    }
}