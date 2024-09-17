#include <cassert>
#include <functional>
#include <iostream>
#include <type_traits>
#include <utility>

auto print_ags(int a) -> int
{
    std::cout << "print_ags:" << a << std::endl;
    return a;
}

struct func_obj
{
    auto operator()(int a) const
    {
        std::cout << "func_obj.operator():" << a << std::endl;
        return a;
    }
};

class Printer
{
    int m_b{1};

  public:
    explicit Printer(int v) : m_b(v) {}
    [[nodiscard]] auto print(int a) const
    {
        std::cout << "Printer.print: " << a << std::endl;
        return a + m_b;
    }
    static auto print_static(int a) // NOLINT
    {
        std::cout << "Printer.print_static: " << a << std::endl;
        return a;
    };
};

template <typename Fun, typename... Args>
using call_result_t = decltype(::std::declval<Fun>()(std::declval<Args>()...));

template <typename Fun, typename... Args>
concept check_transform = requires() { typename call_result_t<Fun, Args...>; };

template <typename Fun, typename... Args>
[[maybe_unused]] consteval bool check_type()
{
    static_assert(check_transform<Fun, Args...>,
                  "pre sender submit Args Unable to call Fun");
    return true;
}

struct func_template
{
    template <typename... Ts>
    auto operator()(Ts... ts) const
    {
        std::cout << "func_template.operator():" << std::endl;
        return (ts + ... + 0);
    }
};

struct A
{
    static int test(double /*unused*/, int /*unused*/)
    {
        std::cout << "A:" << std::endl;
        return 0;
    };

    template <typename... Ts>
    // requires(requires() { typename call_result_t<decltype(A::test), Ts...>; })
    auto operator()(Ts &&...ts) const

    {
        return test(std::forward<Ts>(ts)...);
    }
};
/*
 */
template <typename _T>
struct function_traits;

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...)>
{
    using return_type = _Ret;
    using arg_types = std::tuple<_Args...>;
};
template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const>
{
    using return_type = _Ret;
    using arg_types = std::tuple<_Args...>;
};

// 函数指针的参数类型
template <typename _Ret, typename... _Args>
struct function_traits<_Ret (*)(_Args...)>
{
    using return_type = _Ret;
    using arg_types = std::tuple<_Args...>;
};

template <typename _T>
struct function_info_impl;

template <typename _T>
    requires(std::is_pointer_v<_T>)
struct function_info_impl<_T>
{
    using type = function_traits<_T>;
};

template <typename _T>
    requires(std::is_class_v<_T>) and requires {
        { &_T::operator() };
    }
struct function_info_impl<_T>
{
    using type = function_traits<decltype(&_T::operator())>;
};

template <typename T, typename U>
constexpr bool is_decay_equ = std::is_same_v<std::decay_t<T>, U>; // NOLINT

int main()
{
    // basic test
    {
        // func_ptrs
        {
            auto func_ptr = print_ags;
            assert(func_ptr(1) == 1);
        }
        // func_obj
        {
            func_obj func_obj;
            assert(func_obj(1) == 1);
            // <functional>
            {
                using add_obj = std::plus<>;
                add_obj add;
                assert(add(1, 1) == 2);

                std::greater<> greater;
                assert(greater(5, 3));
            }
        }

        // lambda
        {
            int add = 3;
            auto fun_lambda = [&](int a) {
                std::cout << "fun_lambda: " << a << std::endl;
                return add + a;
            };
            assert(fun_lambda(1) == (add + 1));
        }
        // obj
        {

            Printer printer(2);
            assert(printer.print(1) == (2 + 1));
            assert(printer.print_static(1) == 1);
        }
    }

    std::cout << "\nstd::function: " << std::endl;
    {
        // 使用 std::function 统一调用
        std::function<int(int)> func;

        // 1. 函数指针
        func = print_ags;
        assert(func(1) == 1);

        // 2. 函数对象
        func_obj func_obj;
        func = func_obj;
        assert(func(1) == 1);

        // 3. Lambda 表达式
        int add = 3;
        func = [&](int a) {
            std::cout << "fun_lambda: " << a << std::endl;
            return add + a;
        };
        assert(func(1) == (add + 1));

        // 4. 非静态成员函数
        Printer printer(2);
        func = std::bind(&Printer::print, printer, std::placeholders::_1);
        assert(func(1) == (2 + 1));

        // 5. 静态成员函数
        func = Printer::print_static;
        assert(func(1) == 1);
    }
    // 如何使用 std::function 获得上述函数的 参数和返回值
    // 要求编译期计算
    {
        using T0 = decltype(print_ags);
        using T1 = func_obj;
        using T2 = decltype([](int a) { return a; });
        using T3 = decltype(Printer::print_static);
        // 确定 T0，1，2，3的返回值
        static_assert(not std::is_class_v<T0>);
        static_assert(std::is_class_v<T1>);
        static_assert(std::is_class_v<T2>);
        static_assert(not std::is_class_v<T3>);
        // 返回值
        {
            using T = call_result_t<T0, int>;
            using T = call_result_t<T0, int &>;
            using T = call_result_t<T0, int &&>;
            {
                // using T = call_result_t<T0, int *>; // 错误
                // using T = call_result_t<T0>; // 错误
            } // static_assert(check_transform<T0, int>,
            //               "pre_completion_signature not satify this Fun");
            // static_assert(check_transform<T0>,
            //               " pre_completion_signature not satify this Fun");
            (void)check_type<T0, int>();
            // (void)check_type<T0>();
        }
        // 变参
        {
            using T = call_result_t<func_template, int &&, int, double, float>;
            static_assert(std::is_same_v<double, T>);
            {
                // 隐式转化
                // using T = call_result_t<A, int, double>;
                // using T = call_result_t<A, double, double>;
                // using T = call_result_t<A, int, int>;
                // using T = std::invoke_result_t<A, int, double>;
                // using T = std::invoke_result_t<A, double, double>;
                // using T = std::invoke_result_t<A, int, int>;
                static_assert(not is_decay_equ<double, int>);
                static_assert(not is_decay_equ<int, double>);
                // 我去，上面的太可怕了。 如果func_template，检查不出来

                // 一样可以调用
                //  using T = std::invoke_result_t<decltype(A::test), int, double>;
                //  using T = std::invoke_result_t<decltype(A::test), double, double>;

                static_assert(std::is_class_v<A>);
                using F = function_traits<A>;
                // using Ags = F::arg_types;
                // //模板函数是不可能的，没有声明定义是取不出来的
            }
        }
    }

    std::cout << "\ntest_fun.cpp end\n";
    return 0;
}