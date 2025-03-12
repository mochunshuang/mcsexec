
#include <cassert>
#include <iostream>

struct set_value_t
{
};
constexpr inline set_value_t set_value{}; // NOLINT

int main()
{
    static_assert(not std::is_same_v<decltype(set_value), set_value_t>);
    // NOLINTNEXTLINE
    constexpr auto check = []<class Tag, class... Args>(Tag (*)(Args...)) {
        if constexpr (std::is_same_v<Tag, set_value_t>)
        {
            // Do something
            return true; // NOLINT
        }
        else
        {
            // Do something else
            return false;
        }
    };
    using Sig = set_value_t (*)(int);
    constexpr Sig sig{}; // NOLINT
    static_assert(check(sig));
    {
        using Sig = double (*)(int);
        constexpr Sig sig{}; // NOLINT
        static_assert(not check(sig));
    }

    {
        [[maybe_unused]] constexpr auto check = // NOLINT
            []<class Tag, class... Args>(Tag (*)(Args...)) {
                if constexpr (Tag() == set_value)
                {
                    // Do something
                    return true; // NOLINT
                }
                else
                {
                    // Do something else
                    return false;
                }
            };
        // using Sig = set_value_t (*)(int);
        // constexpr Sig sig{}; // NOLINT
        // static_assert(check(sig));
        // {
        //     using Sig = double (*)(int);
        //     constexpr Sig sig{}; // NOLINT
        //     static_assert(not check(sig));
        // }
        // NOTE: 以上编译错误
        [[maybe_unused]] set_value_t v;
        // assert(v == set_value); // 编译错误，默认是不可比较
    }
    std::cout << "main done\n";
    return 0;
}