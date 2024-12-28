
#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>

decltype(auto) fun0(int v)
{
    int tmp = v;
    int x = v; //
    // return std::move(x); // 错误来源 =>int&&. 太非直觉。int&& =>auto&&  怎么了？？？

    // const int && 就可以......
    // return [x = std::forward<decltype(v)>(v)]() -> decltype(auto) {
    //     return std::move(x); // 正确了，因为返回值是int
    // }();

    // int &&。但是可以
    return [x = std::forward<decltype(v)>(v)]() mutable -> decltype(auto) {
        return std::move(x);
    }();

    // return [x = std::forward<decltype(v)>(v)]() {
    //     return std::move(x); // 正确了，因为返回值是int
    // }();
}
template <typename T>
decltype(auto) fun1(T &&v)
{
    return [x = std::forward<decltype(v)>(v)]() mutable -> decltype(auto) {
        return std::move(x); // 正确了，因为返回值是int
    }();
}

auto fun2(int i)
{
    if (i == 0)
    {
        int test = 1;
        return fun1(test);
    }
    if (i == 1)
    {
        int test = 1;
        return fun1(std::move(test));
    }
    if (i == 2)
    {
        int test = 1;
        int &rv = test;
        return fun1(rv);
    }
    if (i == 3)
    {
        int test = 3;
        int &rv = test;
        return fun1(std::move(rv));
    }
    if (i == 4)
    {
        return fun1(int{4});
    }

    return -1;
}

decltype(auto) fun3(int &v)
{
    // int &&。但是可以
    return [x = std::forward<decltype(v)>(v)]() mutable -> decltype(auto) {
        return std::move(x);
    }();
}

int main()
{
    int value = 0;
    int target = value;

    decltype(auto) ret = fun0(value);

    assert(ret == target);
    std::cout << "main done\n";
    {
        int test = 1;
        decltype(auto) ret = fun1(test);
        static_assert(std::is_same_v<decltype(ret), int &&>);
        assert(ret == test);
        assert(ret == fun2(0));
    }
    {
        int test = 1;
        decltype(auto) ret = fun1(std::move(test));
        static_assert(std::is_same_v<decltype(ret), int &&>);
        assert(ret == 1);
        assert(ret == fun2(1));
    }
    {
        int test = 1;
        int &rv = test;
        decltype(auto) ret = fun1(rv);
        static_assert(std::is_same_v<decltype(ret), int &&>);
        assert(ret == 1);
        assert(ret == fun2(2));
        {
            test = 3;
            decltype(auto) ret = fun1(std::move(rv));
            static_assert(std::is_same_v<decltype(ret), int &&>);
            assert(ret == 3);
            assert(ret == fun2(3));
        }
    }
    {
        // warning: possibly dangling reference to a temporary
        // decltype(auto) ret = fun1(int{4});
        // static_assert(std::is_same_v<decltype(ret), int &&>);
        // assert(ret == 4);
        // assert(ret == fun2(4));

        {
            [[maybe_unused]] auto ret = fun1(int{4}); // 没有警告
        }
        {
            // warning: possibly dangling reference to a temporary [-Wdangling-reference]
            [[maybe_unused]] auto &&ret = fun1(int{4}); // 没有警告
        }

        using T = decltype(((ret)));
        // 值得注意的地方
        static_assert(std::is_same_v<T, int &>); // 左值引用
        {
            int ret = 0;
            using T = decltype(((ret)));
            static_assert(std::is_same_v<T, int &>); // 还是左值引用
        }
    }
    {
        // decltype(auto) ret = fun3(0); // 失败
        int a = 1;
        decltype(auto) ret = fun3(a);
        assert(ret == 1);
    }
    return 0;
}