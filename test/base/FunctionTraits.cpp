#include <iostream>
#include <tuple>

template <typename _T>
struct function_traits;

// 成员函数模板特化
template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...)>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = false;    // NOLINT
    static constexpr bool is_lvalue = false;   // NOLINT
    static constexpr bool is_rvalue = false;   // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = false;   // NOLINT
    static constexpr bool is_lvalue = false;  // NOLINT
    static constexpr bool is_rvalue = false;  // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = true;     // NOLINT
    static constexpr bool is_lvalue = false;   // NOLINT
    static constexpr bool is_rvalue = false;   // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = true;    // NOLINT
    static constexpr bool is_lvalue = false;  // NOLINT
    static constexpr bool is_rvalue = false;  // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) &>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = false;    // NOLINT
    static constexpr bool is_lvalue = true;    // NOLINT
    static constexpr bool is_rvalue = false;   // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) & noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = false;   // NOLINT
    static constexpr bool is_lvalue = true;   // NOLINT
    static constexpr bool is_rvalue = false;  // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const &>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = true;     // NOLINT
    static constexpr bool is_lvalue = true;    // NOLINT
    static constexpr bool is_rvalue = false;   // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const & noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = true;    // NOLINT
    static constexpr bool is_lvalue = true;   // NOLINT
    static constexpr bool is_rvalue = false;  // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) &&>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = false;    // NOLINT
    static constexpr bool is_lvalue = false;   // NOLINT
    static constexpr bool is_rvalue = true;    // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) && noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = false;   // NOLINT
    static constexpr bool is_lvalue = false;  // NOLINT
    static constexpr bool is_rvalue = true;   // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const &&>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = true;     // NOLINT
    static constexpr bool is_lvalue = false;   // NOLINT
    static constexpr bool is_rvalue = true;    // NOLINT
};

template <typename _Ret, typename _Class, typename... _Args>
struct function_traits<_Ret (_Class::*)(_Args...) const && noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = true;    // NOLINT
    static constexpr bool is_lvalue = false;  // NOLINT
    static constexpr bool is_rvalue = true;   // NOLINT
};

// 全局函数模板特化
template <typename _Ret, typename... _Args>
struct function_traits<_Ret (*)(_Args...)>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = false; // NOLINT
    static constexpr bool is_const = false;    // NOLINT
    static constexpr bool is_lvalue = false;   // NOLINT
    static constexpr bool is_rvalue = false;   // NOLINT
};

template <typename _Ret, typename... _Args>
struct function_traits<_Ret (*)(_Args...) noexcept>
{
    using ret_t = _Ret;
    using arg_t = std::tuple<_Args...>;
    static constexpr bool is_noexcept = true; // NOLINT
    static constexpr bool is_const = false;   // NOLINT
    static constexpr bool is_lvalue = false;  // NOLINT
    static constexpr bool is_rvalue = false;  // NOLINT
};

template <typename _T>
struct function_info_impl;

template <typename _T>
struct function_info_impl
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

template <typename _F>
using trait_function = function_info_impl<_F>::type;

// 全局函数
int globalFunction(int a, double b) // NOLINT
{
    return static_cast<int>(a + b);
}
// Note: 无法萃取 函数是部分 static
static int globalFunctionStatic(int a, double b) noexcept // NOLINT
{
    return static_cast<int>(a + b);
}
int globalFunctionRef(int &a, double &b) // NOLINT
{
    return static_cast<int>(a + b);
}
const int &globalFunctionReturnConstRef(const int &a, double b) // NOLINT
{
    static int result = static_cast<int>(a + b);
    return result;
}

// 编译错误
// int globalFunctionConst(int a, double b) const // NOLINT
// {
//     return static_cast<int>(a + b);
// }

// 静态成员函数
struct ExampleClass
{
    // 静态成员函数
    static int staticFunction(int a, double b)
    {
        return static_cast<int>(a + b);
    }

    // 普通成员函数
    int normalFunction(int a, double b) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept 成员函数
    int noexceptFunction(int a, double b) noexcept(true) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept(false) 成员函数
    int exceptFunction(int a, double b) noexcept(false) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // const 成员函数
    int constFunction(int a, double b) const // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // & 成员函数（只能被左值对象调用）
    int lvalueFunction(int a, double b) & // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // && 成员函数（只能被右值对象调用）
    int rvalueFunction(int a, double b) && // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // const & 成员函数（只能被左值对象调用，且不会修改成员变量）
    int constLvalueFunction(int a, double b) const & // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // const && 成员函数（只能被右值对象调用，且不会修改成员变量）
    int constRvalueFunction(int a, double b) const && // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept const 成员函数
    int noexceptConstFunction(int a, double b) const noexcept(true) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept(false) const 成员函数
    int exceptConstFunction(int a, double b) const noexcept(false) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept & 成员函数（只能被左值对象调用）
    int noexceptLvalueFunction(int a, double b) & noexcept(true) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept(false) & 成员函数（只能被左值对象调用）
    int exceptLvalueFunction(int a, double b) & noexcept(false) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept && 成员函数（只能被右值对象调用）
    int noexceptRvalueFunction(int a, double b) && noexcept(true) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept(false) && 成员函数（只能被右值对象调用）
    int exceptRvalueFunction(int a, double b) && noexcept(false) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept const & 成员函数（只能被左值对象调用，且不会修改成员变量）
    int noexceptConstLvalueFunction(int a, double b) const & noexcept(true) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept(false) const & 成员函数（只能被左值对象调用，且不会修改成员变量）
    int exceptConstLvalueFunction(int a, double b) const & noexcept(false) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept const && 成员函数（只能被右值对象调用，且不会修改成员变量）
    int noexceptConstRvalueFunction(int a, double b) const && noexcept(true) // NOLINT
    {
        return static_cast<int>(a + b);
    }

    // noexcept(false) const && 成员函数（只能被右值对象调用，且不会修改成员变量）
    int exceptConstRvalueFunction(int a, double b) const && noexcept(false) // NOLINT
    {
        return static_cast<int>(a + b);
    }
};

void test_global(); // NOLINT
void test_static(); // NOLINT
void test_class();  // NOLINT
void test_lambda(); // NOLINT
void test_void();   // NOLINT
int main()
{
    test_global();
    test_static();
    test_class();
    test_lambda();
    test_void();
    std::cout << "hello world\n";
    return 0;
}
void test_global()
{
    using T = trait_function<decltype(&globalFunction)>;
    static_assert(std::is_same_v<int, T::ret_t>);
    static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
    static_assert(not T::is_noexcept);
    static_assert(not T::is_const);
    {
        using T = trait_function<decltype(&globalFunctionStatic)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(not T::is_const);
    }
    {
        using T = trait_function<decltype(&globalFunctionRef)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int &, double &>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
    }
    {
        using T = trait_function<decltype(&globalFunctionReturnConstRef)>;
        static_assert(std::is_same_v<const int &, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<const int &, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
    }
}
void test_static()
{
    using T = trait_function<decltype(&ExampleClass::staticFunction)>;
    static_assert(std::is_same_v<int, T::ret_t>);
    static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
    static_assert(not T::is_noexcept);
    static_assert(not T::is_const);
}
void test_class()
{
    // staticFunction
    {
        using T = trait_function<decltype(&ExampleClass::staticFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // normalFunction
    {
        using T = trait_function<decltype(&ExampleClass::normalFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // noexceptFunction
    {
        using T = trait_function<decltype(&ExampleClass::noexceptFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // exceptFunction
    {
        using T = trait_function<decltype(&ExampleClass::exceptFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // constFunction
    {
        using T = trait_function<decltype(&ExampleClass::constFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // lvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::lvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // rvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::rvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(T::is_rvalue);
    }

    // constLvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::constLvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // constRvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::constRvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(T::is_rvalue);
    }

    // noexceptConstFunction
    {
        using T = trait_function<decltype(&ExampleClass::noexceptConstFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // exceptConstFunction
    {
        using T = trait_function<decltype(&ExampleClass::exceptConstFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // noexceptLvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::noexceptLvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // exceptLvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::exceptLvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // noexceptRvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::noexceptRvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(T::is_rvalue);
    }

    // exceptRvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::exceptRvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(T::is_rvalue);
    }

    // noexceptConstLvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::noexceptConstLvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(T::is_const);
        static_assert(T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // exceptConstLvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::exceptConstLvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // noexceptConstRvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::noexceptConstRvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(T::is_rvalue);
    }

    // exceptConstRvalueFunction
    {
        using T = trait_function<decltype(&ExampleClass::exceptConstRvalueFunction)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, double>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(T::is_rvalue);
    }
}

void test_lambda()
{
    // 无异常，无mutable，带参数和返回值
    {
        auto lambda = [](int a, int b) -> int {
            return a + b;
        };
        using T = trait_function<decltype(lambda)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, int>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // 无异常，有mutable，带参数和返回值
    {
        auto lambda = [](int a, int b) mutable -> int {
            return a * b;
        };
        using T = trait_function<decltype(lambda)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, int>, T::arg_t>);
        static_assert(not T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // 有异常，无mutable，带参数和返回值
    {
        auto lambda = [](int a, int b) noexcept -> int {
            return a - b;
        };
        using T = trait_function<decltype(lambda)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, int>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }

    // 有异常，有mutable，带参数和返回值
    {
        auto lambda = [](int a, int b) mutable noexcept -> int {
            return a / b;
        };
        using T = trait_function<decltype(lambda)>;
        static_assert(std::is_same_v<int, T::ret_t>);
        static_assert(std::is_same_v<std::tuple<int, int>, T::arg_t>);
        static_assert(T::is_noexcept);
        static_assert(not T::is_const);
        static_assert(not T::is_lvalue);
        static_assert(not T::is_rvalue);
    }
    // 但引用的返回值

    auto lambda = [](int &a, int /*b*/) mutable noexcept -> decltype(auto) {
        return a;
    };
    using T = trait_function<decltype(lambda)>;
    static_assert(std::is_same_v<int &, T::ret_t>);
    static_assert(std::is_same_v<std::tuple<int &, int>, T::arg_t>);

    static_assert(T::is_noexcept);
    static_assert(not T::is_const);
    static_assert(not T::is_lvalue);
    static_assert(not T::is_rvalue);
}

// 全局函数
void global_void_void() {}       // NOLINT
void global_void_nonvoid(int) {} // NOLINT
int global_nonvoid_void()        // NOLINT
{
    return 0;
}

// 成员函数
struct MyClass
{
    void member_void_void() {}       // NOLINT
    void member_void_nonvoid(int) {} // NOLINT
    int member_nonvoid_void()        // NOLINT
    {
        return 0;
    }
};

// Lambda 表达式
// NOLINTNEXTLINE
auto lambda_void_void = []() {
};
// NOLINTNEXTLINE
auto lambda_void_nonvoid = [](int) {
};
// NOLINTNEXTLINE
auto lambda_nonvoid_void = []() {
    return 0;
};
void test_void()
{
    // 测试全局函数
    {
        using T1 = trait_function<decltype(&global_void_void)>;
        static_assert(std::is_same_v<void, T1::ret_t>);
        static_assert(std::is_same_v<std::tuple<>, T1::arg_t>);
        static_assert(not T1::is_noexcept);
        static_assert(not T1::is_const);

        using T2 = trait_function<decltype(&global_void_nonvoid)>;
        static_assert(std::is_same_v<void, T2::ret_t>);
        static_assert(std::is_same_v<std::tuple<int>, T2::arg_t>);
        static_assert(not T2::is_noexcept);
        static_assert(not T2::is_const);

        using T3 = trait_function<decltype(&global_nonvoid_void)>;
        static_assert(std::is_same_v<int, T3::ret_t>);
        static_assert(std::is_same_v<std::tuple<>, T3::arg_t>);
        static_assert(not T3::is_noexcept);
        static_assert(not T3::is_const);
    }

    // 测试成员函数
    {
        using T1 = trait_function<decltype(&MyClass::member_void_void)>;
        static_assert(std::is_same_v<void, T1::ret_t>);
        static_assert(std::is_same_v<std::tuple<>, T1::arg_t>);
        static_assert(not T1::is_noexcept);
        static_assert(not T1::is_const);

        using T2 = trait_function<decltype(&MyClass::member_void_nonvoid)>;
        static_assert(std::is_same_v<void, T2::ret_t>);
        static_assert(std::is_same_v<std::tuple<int>, T2::arg_t>);
        static_assert(not T2::is_noexcept);
        static_assert(not T2::is_const);

        using T3 = trait_function<decltype(&MyClass::member_nonvoid_void)>;
        static_assert(std::is_same_v<int, T3::ret_t>);
        static_assert(std::is_same_v<std::tuple<>, T3::arg_t>);
        static_assert(not T3::is_noexcept);
        static_assert(not T3::is_const);
    }

    // 测试 Lambda 表达式
    {
        using T1 = trait_function<decltype(lambda_void_void)>;
        static_assert(std::is_same_v<void, T1::ret_t>);
        static_assert(std::is_same_v<std::tuple<>, T1::arg_t>);
        static_assert(not T1::is_noexcept);
        static_assert(T1::is_const);

        using T2 = trait_function<decltype(lambda_void_nonvoid)>;
        static_assert(std::is_same_v<void, T2::ret_t>);
        static_assert(std::is_same_v<std::tuple<int>, T2::arg_t>);
        static_assert(not T2::is_noexcept);
        static_assert(T2::is_const);

        using T3 = trait_function<decltype(lambda_nonvoid_void)>;
        static_assert(std::is_same_v<int, T3::ret_t>);
        static_assert(std::is_same_v<std::tuple<>, T3::arg_t>);
        static_assert(not T3::is_noexcept);
        static_assert(T3::is_const);
    }
}