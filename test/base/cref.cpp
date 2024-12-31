#include <iostream>

int main()
{
    struct A
    {
        int a{0};
        int b{1};
    };
    auto fun [[maybe_unused]] = [](const A &obj) {
        // auto &  保留 c v ref
        auto &[a, b] = obj;
        static_assert(std::is_same_v<decltype(a), const int>);
    };
    return 0;
}