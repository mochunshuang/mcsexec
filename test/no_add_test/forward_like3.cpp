#include <iostream>
#include <utility>
#include <type_traits>
#include <string>
#include <tuple>

// NOLINTBEGIN
// 辅助函数：获取类型名称
template <typename T>
constexpr const char *get_type_name()
{
    std::string_view name = __PRETTY_FUNCTION__;
#ifdef _MSC_VER
    name.remove_prefix(name.find('<') + 1);
    name.remove_suffix(name.size() - name.find('>'));
#else
    name.remove_prefix(name.find("= ") + 2);
    name.remove_suffix(1);
#endif
    return name.data();
}

// 辅助函数：打印转发结果类型
template <typename T>
void print_forward_result(const char *scenario)
{
    std::cout << scenario << ":\n";
    std::cout << "  推导类型: " << get_type_name<T>() << "\n";
    std::cout << "  是左值引用? " << std::boolalpha
              << std::is_lvalue_reference_v<T> << "\n";
    std::cout << "  是右值引用? " << std::boolalpha
              << std::is_rvalue_reference_v<T> << "\n";
    std::cout << "  是const? " << std::boolalpha
              << std::is_const_v<std::remove_reference_t<T>> << "\n\n";
}

// 示例1：基础类型转发
void basic_type_demo()
{
    std::cout << "=== 基础类型转发示例 ===\n";
    int x = 42;
    const int y = 100;

    // 1.1 左值按右值引用转发
    auto &&r1 = std::forward_like<int &&>(x);
    print_forward_result<decltype(r1)>("1.1 左值int按int&&转发");

    // 1.2 右值按左值引用转发
    auto &&r2 = std::forward_like<int &>(std::move(x));
    print_forward_result<decltype(r2)>("1.2 右值int按int&转发");

    // 1.3 非const左值按const左值引用转发
    auto &&r3 = std::forward_like<const int &>(x);
    print_forward_result<decltype(r3)>("1.3 非const左值int按const int&转发");

    // 1.4 const左值按非const右值转发
    auto &&r4 = std::forward_like<int &&>(y);
    print_forward_result<decltype(r4)>("1.4 const左值int按int&&转发");
}

// 示例2：复杂类型转发
void complex_type_demo()
{
    std::cout << "=== 复杂类型转发示例 ===\n";
    std::string s = "hello";
    const std::string cs = "world";

    // 2.1 左值字符串按右值引用转发
    auto &&r1 = std::forward_like<std::string &&>(s);
    print_forward_result<decltype(r1)>("2.1 左值string按string&&转发");

    // 2.2 右值字符串按左值引用转发
    auto &&r2 = std::forward_like<std::string &>(std::string("temporary"));
    print_forward_result<decltype(r2)>("2.2 右值string按string&转发");

    // 2.3 非const左值按const右值转发
    auto &&r3 = std::forward_like<const std::string &&>(s);
    print_forward_result<decltype(r3)>("2.3 非const左值string按const string&&转发");
}

// 示例3：模板函数中的转发
template <typename Target, typename Source>
void template_forward_demo(Source &&source)
{
    auto &&val = std::forward_like<Target>(source);
    print_forward_result<decltype(val)>("3. 模板函数中按Target类型转发");
}

// 示例4：元组元素转发
void tuple_element_demo()
{
    std::cout << "=== 元组元素转发示例 ===\n";
    std::tuple<int, const std::string, double> t(10, "test", 3.14);

    // 4.1 按元组声明类型转发第一个元素
    auto &&r1 = std::forward_like<std::tuple_element_t<0, decltype(t)>>(std::get<0>(t));
    print_forward_result<decltype(r1)>("4.1 元组第一个元素按int转发");

    // 4.2 按元组声明类型转发第二个元素
    auto &&r2 = std::forward_like<std::tuple_element_t<1, decltype(t)>>(std::get<1>(t));
    print_forward_result<decltype(r2)>("4.2 元组第二个元素按const string转发");
}

// 示例5：转发与函数重载
void overloaded_func(int &)
{
    std::cout << "  调用了左值引用重载\n";
}
void overloaded_func(int &&)
{
    std::cout << "  调用了右值引用重载\n";
}
void overloaded_func(const int &)
{
    std::cout << "  调用了const左值引用重载\n";
}

void overload_resolution_demo()
{
    std::cout << "=== 转发与函数重载示例 ===\n";
    int x = 5;
    const int cx = 10;

    std::cout << "5.1 左值按右值转发: ";
    overloaded_func(std::forward_like<int &&>(x));

    std::cout << "5.2 右值按左值转发: ";
    overloaded_func(std::forward_like<int &>(std::move(x)));

    std::cout << "5.3 非const按const转发: ";
    overloaded_func(std::forward_like<const int &>(x));

    std::cout << "5.4 const按非const右值转发: ";
    overloaded_func(std::forward_like<int &&>(cx));
    std::cout << "\n";
}

int main()
{
    basic_type_demo();
    complex_type_demo();

    std::cout << "=== 模板函数转发示例 ===\n";
    int a = 100;
    template_forward_demo<long &&>(a); // int左值按long&&转发

    tuple_element_demo();
    overload_resolution_demo();

    /*
//NOTE: 按模板参数转发，与原始类型无关。 不是完美转发，是按需转发
std::forward_like 的核心价值在于：

完全脱离源对象的原始类型和值类别，仅根据目标类型进行转发
可以强制改变值类别（左值↔右值）和 const 属性
在泛型编程和容器元素操作中特别有用
弥补了 std::forward 依赖源类型的局限性
*/
    return 0;
}
// NOLINTEND