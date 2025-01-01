
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