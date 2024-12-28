#include <iostream>

int main()
{
    struct A
    {
    };
    struct B
    {
    };

    constexpr bool k_r = noexcept(noexcept(A{}) && noexcept(B{}));

    static_assert(k_r);

    []<typename... As>(As &&...ags) {
        // noexcept(noexcept(ags) && ...) 是错误的
        constexpr bool k_res = (noexcept(ags) && ...);
        if constexpr ((noexcept(ags) && ...))
        {
            std::cout << k_res << " \n";
        }
        else
        {
            std::cout << "false " << " \n";
        }
    }(A{}, B{});

    std::cout << "hello world\n";
    return 0;
}