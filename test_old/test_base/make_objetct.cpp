#include <type_traits>
#include <utility>
#include <tuple>

struct A
{
    A(const A &) = delete;
    A &operator=(const A &) = delete;
    A &operator=(A &&) = delete;
    A(A &&) = delete;
    ~A() = default;
    explicit A(int a) : a{a} {}
    int a;
};
struct B
{
    B(const B &) = delete;
    B &operator=(const B &) = delete;
    B &operator=(B &&) = delete;
    B(B &&) = delete;
    ~B() = default;
    explicit B(int a, int b) : a{a}, b{b} {}
    int a;
    int b;
};
A getA(int v)
{
    return A{v};
}
B getB()
{
    return B(1, 2);
};

struct C
{
    A a;
    B b;
};

template <class U, class F>
struct __objetc_t
{
    operator U() // NOLINT
    {
        return f();
    }
    F f; // NOLINT
};

template <class U, class F>
__objetc_t<U, std::decay_t<F>> __make_objetct(F &&f)
{
    return {std::forward<F>(f)};
}

template <typename... Ts>
struct Sender
{
    std::tuple<Ts...> data;
    constexpr explicit Sender(Ts &&...ts) : data{std::forward<Ts>(ts)...} {}
};

template <typename T>
struct no_move_no_copy
{
    T v;
};

template <typename T>
struct no_move_no_copy_2
{
    T v;

    explicit no_move_no_copy_2(T v_) : v(v_) {}
};

void test_no_move_no_copy();

int main()
{
    int value1 = 0;
    int value2 = 1;
    std::tuple<A, A, A> t(A(value1), A(value2));
    {
        auto v = std::tuple<A, A, A>(1, 2, 3); // 可以
        // auto v2 = std::tuple<A, A, A>(A{1}, A{2}, A{3}); // 不行
        // std::tuple<A, A, A> v3; // 不行
        C c(A{1}, B{1, 2}); // 可以
        C(A{1}, B{1, 2});
    }
    // 下面的复杂了
    (void)t;
    {
        std::tuple<A> t(__make_objetct<A>([]() { return A{1}; }));
    }

    {
        std::tuple<A> t(__make_objetct<A>([]() { return getA(1); }));
    }

    {
        std::tuple<A, B> t(__make_objetct<A>([]() { return getA(1); }),
                           __make_objetct<B>([]() { return getB(); }));
    }

    {
        std::tuple<A, B, C> t(__make_objetct<A>([]() { return getA(1); }),
                              __make_objetct<B>([]() { return getB(); }),
                              __make_objetct<C>([]() { return C(A{1}, B(1, 2)); }));
    }
    {
        // 转发都用不了,什么都用不了。移动构造不能没有
        // Sender<A, B, C> s; // 不行
        // 使用 __make_objetct 生成 A, B, C 对象
        auto a_obj = __make_objetct<A>([]() { return getA(1); });
        auto b_obj = __make_objetct<B>([]() { return getB(); });
        auto c_obj = __make_objetct<C>([]() { return C(A{1}, B{1, 2}); });

        // // 初始化 Sender<A, B, C>
        // Sender<A, B, C> s(a_obj, b_obj, c_obj); //不可能
    }
    test_no_move_no_copy();
    return 0;
}
void test_no_move_no_copy()
{
    [[maybe_unused]] auto a = no_move_no_copy(A{0});
    {
        // Note: 最好一个构造函数都不写
        //  [[maybe_unused]] auto a = no_move_no_copy_2(A{0});
    }
}