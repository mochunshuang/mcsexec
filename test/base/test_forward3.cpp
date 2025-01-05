#include <cassert>

#include <type_traits>
#include <utility>

template <typename T>
struct product_type
{
    std::decay_t<T> value;
};
template <typename T>
product_type(T &&) -> product_type<std::decay_t<T>>; // NOLINT

template <typename T>
auto make_type(T &&t) // NOLINT
{
    return product_type<std::decay_t<T>>{std::forward<T>(t)};
}

template <typename T>
auto make_type2(T &&t) // NOLINT
{
    return product_type<std::decay_t<T>>{static_cast<std::decay_t<T> &&>(t)};
}

template <typename T>
decltype(auto) make_type3(T &&t) // NOLINT
{
    return product_type<std::decay_t<T>>{static_cast<std::decay_t<T> &&>(t)};
}

struct move_only_type
{
    move_only_type() : val(0) {}
    explicit move_only_type(int v) : val(v) {}
    ~move_only_type() = default;

    move_only_type(const move_only_type &) = delete;
    move_only_type &operator=(const move_only_type &) = delete;

    move_only_type &operator=(move_only_type &&) = default;
    move_only_type(move_only_type &&) = default;
    int val; // NOLINT
};

template <typename T>
auto forward_value(T &&t) // NOLINT
{
    return std::forward<T>(t);
}
template <typename T>
decltype(auto) forward_value1(T &&t) // NOLINT
{
    return std::forward<T>(t);
}

void test0(); // NOLINT
void test1(); // NOLINT
int main()
{
    test0();
    test1();
    return 0;
}
void test0()
{
    {
        // auto ret = make_type(move_only_type{1});
        // auto r = make_type(ret); //编译失败，不能引用
    }
    {
        // auto &&ret = make_type(move_only_type{1});
        // auto r = make_type(ret); // 编译失败，不能引用
    }
    {
        // Note: 计时 make_type的返回值 是 auto 。 但是还是可以 auto && 接收纯右值
        auto &&ret = make_type(move_only_type{1});
        static_assert(std::is_rvalue_reference_v<decltype(ret)>);
        auto r [[maybe_unused]] = make_type(std::forward<decltype(ret)>(ret)); // 成功
    }
    {
        // auto &&ret = make_type(move_only_type{1});
        // using T = decltype((ret));
        // auto r [[maybe_unused]] = make_type(std::forward<T>(ret)); // 失败
    }
    {
        auto &&ret = make_type(move_only_type{1});
        using T = decltype((ret));
        auto r [[maybe_unused]] = make_type2(std::forward<T>(ret)); // 成功
    }
    // Note: 好像无论如何 move_only 的A&  => std::decay_t<T> 只能走移动
    {
        auto a = move_only_type{1};
        auto &&rf = a;
        static_assert(std::is_lvalue_reference_v<decltype(rf)>);
        // 完美转发，左引用转发，失败的，正常
        // make_type(std::forward<decltype((rf))>(rf));
        auto b = make_type(std::forward<decltype(a)>(a)); // 成功，引用成了移动
        assert(b.value.val == 1);
    }
}
void test1()
{
    {
        // auto ret = forward_value(move_only_type{}); //失败 auto 对move_only 不友好
        auto ret1 = forward_value1(move_only_type{});
    }
    {
        auto &&ref = move_only_type{};
        // auto ret = forward_value(ref); // 失败，不能移动vv
        // auto ret1 = forward_value1(ref);
        auto &ret1 = forward_value1(ref);
        auto &&ret2 = forward_value1(ref);
        // Note: 说明 decltype(auto) 是 计算一次的关键。
        assert(&ret1 == &ret2);
    }

    {
        auto obj = move_only_type{};
        auto &ref = obj;
        auto &ret1 = forward_value1(ref);
        auto &&ret2 = forward_value1(ref);
        assert(&ret1 == &obj);
        assert(&ret1 == &ret2);
    }
    {
        auto obj [[maybe_unused]] = make_type3(move_only_type{});
        auto &ref = obj;
        auto &ret1 = forward_value1(ref);
        assert(&ret1 == &obj);
    }
    {
        auto obj [[maybe_unused]] = make_type3(move_only_type{});
        auto &ref = obj;
        auto &ret1 = forward_value1(ref);
        assert(&ret1 == &obj);
    }
    {
        auto &&ref = make_type3(move_only_type{});

        auto &ret1 = forward_value1(ref);
        assert(&ret1 == &ref);
    }
    {
        auto obj [[maybe_unused]] = make_type3(move_only_type{});
        auto &&ref = make_type3(std::move(obj));
        static_assert(std::is_same_v<decltype(ref.value), decltype(obj)>);
        assert(&(ref.value) != &obj); // 失败为何？不是移动吗？指向的不是同一个吗

        auto &ret1 = forward_value1(ref);
        // Note: 引用指向同一个对象，因此两个引用取地址比较可以相等
        // Note: 两个对象的，取地址比较一定不相等吗
        assert(&ret1 == &ref);
    }
    {
        auto obj [[maybe_unused]] = make_type3(move_only_type{});
        auto obj2 = std::move(obj);
        assert(&obj2 != &obj); // Note: 两个对象，地址肯定是不同的
    }
    {
        // Note: 两个对象的，取地址比较一定不相等
        // Note: 地址 和 物理内存挂钩的。对象占用物理内存的地址空间
        constexpr int a = 1; // NOLINT
        constexpr int b = 1; // NOLINT
        assert(&a != &b);
    }
    {
        // auto &ref; // 引用一定要初始化
        int a = 0;
        auto &ref = a;
        [&](int a) {
            assert(&a != &ref);
        }(a);
    }
    {
        // auto &ref; // 引用一定要初始化
        int a = 0;
        auto &ref = a;
        auto ret = [&](int a) {
            ref = a; // 修改指向
            return a;
        }(a);
        assert(&ret != &ref);
    }
    {
        // auto &ref; // 引用一定要初始化
        int a = 0;
        auto &ref = a;
        auto ret = [&](int a) -> decltype(auto) {
            ref = a; // 修改指向
            return a;
        }(a);
        assert(&ret != &ref);
    }
    {
        // auto &ref; // 引用一定要初始化
        int a = 0;
        auto &ref = a;
        auto &&ret = [&](int a) -> decltype(auto) {
            ref = a; // 修改指向
            return std::move(a);
        }(a);
        assert(&ret != &ref);
    }
    {
        // Note: 可以完美计算 + 转发
        int a = 0;
        auto &ref = a;
        auto &&ret = [&](auto &&a) -> decltype(auto) {
            ref = a;
            return std::forward<decltype(a)>(a);
        }(a);
        assert(&ret == &ref);
        assert(&ret == &a);
    }
    {
        // Note: 可以完美计算 + 转发
        int a = 0;
        auto &ref = a;
        auto &&ret = [&]<typename T>(T &&a) -> decltype(auto) {
            ref = a;
            return std::forward<T>(a);
        }(a);
        assert(&ret == &ref);
        assert(&ret == &a);
    }
    {
        // Note: 可以完美计算 + 转发
        int a = 0;
        auto &ref = a;
        auto &&ret = [&]<typename T>(T &&a) -> decltype(auto) {
            ref = a;
            using S = decltype((a));
            return std::forward<S>(a);
        }(a);
        assert(&ret == &ref);
        assert(&ret == &a);
    }
}
