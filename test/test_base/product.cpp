#include "../../include/execution.hpp"
#include "__cout_receiver.hpp"
#include "__my_class.hpp"

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <utility>

struct Tag
{
};
void test0();
void test1();
void test2();
void test3();
void test4();

int main()
{
    test0();

    test1();

    test2();

    test3();
    test4();

    std::cout << "hello world\n";
    return 0;
}

void test0()
{
    using namespace mcs::execution;

    static_thread_pool<3> thread_pool;
    scheduler auto sched6 = thread_pool.get_scheduler();
    sender auto start = schedule(sched6);
    auto product = snd::__detail::product_type{Tag{}, std::move(start)};
    [[maybe_unused]] auto &[a, b] = product;

    static_assert(not std::is_lvalue_reference_v<decltype(product)>);

    auto fun_ret_ref = [](auto &&p) -> decltype(auto) {
        static_assert(std::is_lvalue_reference_v<decltype(p)>);
        return p;
    };
    decltype(auto) rv = fun_ret_ref(product);
    static_assert(std::is_lvalue_reference_v<decltype(rv)>);

    auto fun_debind = [](auto &&p) -> decltype(auto) {
        static_assert(std::is_lvalue_reference_v<decltype(p)>);
        [[maybe_unused]] auto &[a, b] = p;
    };
    fun_debind(std::forward<decltype(rv)>(rv));
}

void test1()
{
    using namespace mcs::execution;

    static_thread_pool<3> thread_pool;
    scheduler auto sched6 = thread_pool.get_scheduler();
    sender auto start = schedule(sched6);
    auto product =
        snd::__detail::product_type{Tag{}, thread_pool.get_scheduler(), std::move(start)};
    [[maybe_unused]] auto &[a, b, c] = product;

    // 继续
    auto snd_read = just(1, 2, 3) | then([](int a, int b, int c) {
                        std::cout << a << " " << b << " " << c
                                  << " thread_id: " << std::this_thread::get_id() << '\n';
                    });
    // [[maybe_unused]] auto &[h, i, j] = snd_read; // 失败
    // 嵌套
    auto product_2 = snd::__detail::product_type{Tag{}, product, snd_read};
    {
        [[maybe_unused]] auto &[a, b, c] = product_2;
        [[maybe_unused]] auto &[d, e, f] = b;
        // [[maybe_unused]] auto &[h, i, j] = c; // 失败
    }
}

template <typename T>
struct test_bind
{
    T value; // NOLINT

    // 默认构造函数
    test_bind() = default;
    ~test_bind() = default;

    // 拷贝构造函数
    test_bind(const test_bind &other) : value(other.value)
    {
        std::cout << "test_bind(const test_bind &other)\n";
    }

    // 移动构造函数
    test_bind(test_bind &&other) noexcept : value(std::move(other.value))
    {
        std::cout << "test_bind(test_bind &&other)\n";
    }

    // 拷贝赋值运算符
    test_bind &operator=(const test_bind &other)
    {
        if (this != &other)
        {
            value = other.value;
            std::cout << "test_bind &operator=(const test_bind &other)\n";
        }
        return *this;
    }

    // 移动赋值运算符
    test_bind &operator=(test_bind &&other) noexcept
    {
        if (this != &other)
        {
            value = std::move(other.value);
            std::cout << "test_bind &operator=(test_bind &&other)\n";
        }
        return *this;
    }

    // 带参数的构造函数
    explicit test_bind(T val) : value(val)
    {
        std::cout << "explicit test_bind(T val)\n";
    }
};
void test2()
{
    using namespace mcs::execution;
    auto snd_read = just(1, 2, 3);

    auto &[a, b] = snd_read; // 失败

    {
        std::cout << "\nbasic_sender: test_bind\n";
        auto args =
            mcs::execution::snd::__detail::basic_sender<test_bind<int>, test_bind<int>>{
                test_bind<int>{1}, test_bind<int>{2}};

        auto [a, b] = args;   // copy
        auto &[c, d] = args;  // no copy
        auto &&[e, f] = args; // move

        // auto &[g] = args; // 不能截断

        static_assert(not std::is_same_v<decltype(e), test_bind<int> &&>);
        std::cout << "\nbasic_sender: test_bind end\n";
    }
    {
        std::cout << "\nbasic_sender: test_apply\n";

        auto args =
            mcs::execution::snd::__detail::basic_sender<test_bind<int>, test_bind<int>>{
                test_bind<int>{1}, test_bind<int>{2}};

        std::cout << "\nbasic_sender: test_apply start \n";
        using T = decltype(static_cast<decltype(args)>(args));
        static_assert(
            std::is_same_v<mcs::execution::snd::__detail::basic_sender<test_bind<int>,
                                                                       test_bind<int>>,
                           T>);
        using LT =
            mcs::execution::snd::__detail::basic_sender<test_bind<int>, test_bind<int>> &;
        using RT =
            mcs::execution::snd::__detail::basic_sender<test_bind<int>, test_bind<int>>
                &&;
        // Note: forward_like: copy。 forward: copy and move
        static_cast<T>(args).apply([](auto &&a, auto &&b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end T\n";
        });

        // Note: forward_like: Nothing。
        static_cast<LT>(args).apply([](auto &&a, auto &&b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end LT\n";
        });

        // Note: forward_like: Nothing。 because of parm has name value
        static_cast<RT>(args).apply([](auto &&a, auto &&b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end RT\n";
        });

        // Note: forward_like:
        static_cast<LT>(args).apply([](auto &a, auto &b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end RT\n";
        });

        std::cout << "\nbasic_sender: test_apply end\n";
    }
}

void test3()
{
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};
    std::cout << "\ntest3: just\n";
    auto task = just(std::move(partials));
    auto &&[_, data] = task;
    std::cout << "\nauto &&[_, data] = task end\n";
    {
        // Note: sndr = 是复制
        using OutSndr = decltype((task));
        auto fun = [sndr = std::forward_like<OutSndr>(data)]() {
        };
    }
    std::cout << "\n[]<typename Sndr>(Sndr &&out_sndr) -> decltype(auto)\n";
    {
        // Note: 没有影响
        decltype(auto) fun = []<typename Sndr>(Sndr &&out_sndr) -> decltype(auto) {
            using OutSndr = decltype((out_sndr));
            auto &&[_, sndr] = out_sndr;
            return [sndr = std::forward_like<OutSndr>(sndr)]() mutable noexcept(
                       std::is_nothrow_move_constructible_v<decltype(sndr)>) {
                return std::move(sndr);
            };
        }(std::move(task));
        static_assert(not std::is_lvalue_reference_v<decltype(fun)>);
        static_assert(not std::is_rvalue_reference_v<decltype(fun)>);
        static_assert(
            not std::is_same_v<decltype(fun),
                               snd::__detail::product_type<std::vector<MyClass>>>);

        static_assert(std::is_same_v<decltype(fun()),
                                     snd::__detail::product_type<std::vector<MyClass>>>);

        std::cout << "\nuse sndr data \n";

        cout_receiver rcvr{};
        using Completion = set_value_t;

        // TODO 因此，右值的输出和左值是不兼容的，容易出错
        //  Note: fun() 是没有带const的
        //  Note: e:\0_github_project\mcsexec\mcsexec\test\test_base\product.cpp:247:13:
        //  error: cannot bind non-const lvalue reference of type
        //  'std::vector<MyClassT<int> >&' to an rvalue of type 'std::vector<MyClassT<int>
        //  >'
        //  fun().apply(
        //      [&rcvr](auto &...ts) { Completion()(std::move(rcvr), std::move(ts)...);
        //      });

        auto sndr = fun();
        static_assert(std::is_same_v<decltype(sndr),
                                     snd::__detail::product_type<std::vector<MyClass>>>);
        sndr.apply(
            [&rcvr](auto &...ts) { Completion()(std::move(rcvr), std::move(ts)...); });
    }
}

void test4()
{
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};
    std::cout << "\ntest4: just\n";
    auto task = just(std::move(partials));
}