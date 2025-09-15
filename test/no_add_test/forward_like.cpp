#include <iostream>
#include <utility>
#include <type_traits>
// NOLINTBEGIN
// 辅助函数：打印参数类型和值
template <typename T>
void print_info(T &&val, const char *type_name)
{
    std::cout << type_name << " - 类型: ";
    if constexpr (std::is_lvalue_reference_v<T>)
    {
        std::cout << "左值引用";
    }
    else if constexpr (std::is_rvalue_reference_v<T>)
    {
        std::cout << "右值引用";
    }
    else
    {
        std::cout << "值类型";
    }
    std::cout << ", 值: " << val << "\n";
}

// 使用std::forward的转发函数
template <typename T>
void forward_demo(T &&t)
{
    // std::forward根据原始类型转发
    auto &&forwarded = std::forward<T>(t);
    print_info(std::forward<T>(t), "std::forward结果");
}

// 使用std::forward_like的转发函数
template <typename T, typename U>
void forward_like_demo(U &&u)
{
    // std::forward_like忽略u的原始类型，按T的类型转发
    auto &&liked = std::forward_like<T>(u);
    print_info(std::forward_like<T>(u), "std::forward_like结果");
}

int main()
{
    int x = 42;
    const int y = 100;

    std::cout << "=== 测试std::forward的行为 ===\n";
    std::cout << "1. 转发左值x:\n";
    forward_demo(x); // 左值引用转发

    std::cout << "\n2. 转发右值std::move(x):\n";
    forward_demo(std::move(x)); // 右值引用转发

    std::cout << "\n3. 转发const左值y:\n";
    forward_demo(y); // const左值引用转发

    std::cout << "\n\n=== 测试std::forward_like的行为 ===\n";
    std::cout << "1. 把左值x按int&&类型转发:\n";
    forward_like_demo<int &&>(x); // 强制转为右值引用

    std::cout << "\n2. 把右值std::move(x)按int&类型转发:\n";
    forward_like_demo<int &>(std::move(x)); // 强制转为左值引用

    std::cout << "\n3. 把非const左值x按const int&类型转发:\n";
    forward_like_demo<const int &>(x); // 强制转为const左值引用

    std::cout << "\n4. 把const左值y按int类型转发:\n";
    forward_like_demo<int>(y); // 强制转为值类型（会触发拷贝）

    return 0;
}
// NOLINTEND