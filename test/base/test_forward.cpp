#include <iostream>
#include <type_traits>
#include <utility>

// 辅助函数：打印类型信息
template <typename T>
void print_type_info(const std::string &name) // NOLINT
{
    std::cout << name
              << " is: " << (std::is_const_v<std::remove_reference_t<T>> ? "const " : "")
              << (std::is_lvalue_reference_v<T> ? "lvalue ref " : "")
              << (std::is_rvalue_reference_v<T> ? "rvalue ref " : "") << typeid(T).name()
              << "\n";
}

// 测试函数模板
template <typename Adaptor>
void test_forward(Adaptor &&adaptor) // NOLINT
{
    std::cout << "--- Testing adaptor ---\n";
    print_type_info<Adaptor>("Adaptor type");
    print_type_info<decltype(adaptor)>("adaptor type (before forward)");

    // 使用 std::forward 转发
    auto &&forwarded_adaptor = std::forward<Adaptor>(adaptor);
    print_type_info<decltype(forwarded_adaptor)>("adaptor type (after forward)");

    // Note: 不保证 成立
    //  static_assert(std::is_same_v<decltype((adaptor)), Adaptor>);
    // Note: 保证一定是左值引用
    static_assert(std::is_lvalue_reference_v<decltype((adaptor))>);
    std::cout << "\n";
}

template <typename T>
void some_function(T &&arg)
{
    if (std::is_lvalue_reference_v<decltype(arg)>)
        std::cout << "Called with lvalue\n";
    else
        std::cout << "Called with rvalue\n";
}

template <typename Adaptor>
void test_forward_2(Adaptor &&adaptor) // NOLINT
{
    // 方式 1：使用 std::forward<Adaptor>(adaptor)
    some_function(std::forward<Adaptor>(adaptor));

    // 方式 2：使用 std::forward<decltype((adaptor))>(adaptor)
    some_function(std::forward<decltype((adaptor))>(adaptor));
}

void base();   // NOLINT
void base_1(); // NOLINT
// 测试用例
int main()
{
    base();
    std::cout << '\n'
              << "=====================" << '\n'
              << "=====================" << '\n';
    base_1();
    return 0;
}

void base()
{
    int value = 42;              // NOLINT
    const int const_value = 100; // NOLINT

    // 测试左值
    std::cout << "lvalue/const: " << "\n";
    test_forward(value);       // 非 const 左值
    test_forward(const_value); // const 左值

    // 测试右值
    std::cout << "rvalue/const: " << "\n";
    test_forward(42);                     // 非 const 右值 // NOLINT
    test_forward(std::move(value));       // 非 const 右值（通过 std::move） // NOLINT
    test_forward(std::move(const_value)); // const 右值（通过 std::move）// NOLINT

    // 测试引用
    std::cout << "Lref/const: " << "\n";
    int &ref = value;
    const int &const_ref = const_value;
    test_forward(ref);       // 非 const 左值引用
    test_forward(const_ref); // const 左值引用

    // 测试右值引用
    std::cout << "Rref/const: " << "\n";
    int &&rref = std::move(value);
    const int &&const_rref = std::move(const_value);
    test_forward(rref);       // 非 const 右值引用
    test_forward(const_rref); // const 右值引用
}
void base_1()
{
    int x = 1;

    // 测试左值
    std::cout << "Testing lvalue:\n";
    test_forward_2(x);

    // 测试右值
    std::cout << "Testing rvalue:\n";
    test_forward_2(1);

    // Note: std::forward<decltype((adaptor))>(adaptor) 保证接下来都是 lvalue
    // Note: 好处也许是 全局 lvalue
}