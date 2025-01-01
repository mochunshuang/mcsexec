

#include <algorithm>
#include <cassert>
#include <forward_list>
#include <tuple>
#include <utility>
int main()
{
    struct A
    {
        int a{0}; // NOLINT
        int b{0}; // NOLINT

        bool operator==(const A &) const = default; // C++20 默认实现
    };
    A obj;
    // Note: forward_like 自带 lint 红色错误. 因为clang 都编译不了
    // Note: 解决 -fno-builtin-std-forward_like
    [](auto &&...ts) {
    }(std::forward_like<decltype(obj)>(obj.a), std::forward_like<decltype(obj)>(obj.b));

    auto ret = []<typename T>(T &&obj) {
        auto t = A(std::forward_like<T>(obj.a), std::forward_like<T>(obj.b));
        return t;
    }(std::move(obj));

    assert(obj == ret);

    return 0;
}