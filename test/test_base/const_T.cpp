#include <iostream>
#include <type_traits>
#include <utility>

// 定义一个概念，检查类型是否是常量引用或常量
template <typename T>
concept const_type = std::is_const_v<std::remove_reference_t<T>>;

// 使用概念约束模板函数
template <const_type T>
// requires(std::is_lvalue_reference_v<T>) //Note: 不需要，一定是
void printValue(T &&value)
{
    decltype(auto) v{std::forward<T>(value)};
    // std::cout << value << std::endl;
}

int test()
{
    return 0;
};

int main()
{
    int x = 5;
    const int y = 10;

    printValue(std::as_const(x)); // 输出 5
    printValue(y);                // 输出 10
    // printValue(x);                // 错误：编译错误
    auto &rx = x;
    // printValue(rx); // 编译错误
    printValue(std::as_const(rx));
    const auto &ry = y;
    printValue(ry);
    printValue(std::as_const(ry));

    // Duplicate 'const' declaration specifier. const 只能一份
    static_assert(std::is_same_v<const int &, decltype(std::as_const(ry))>);
    static_assert(std::is_same_v<decltype(ry), decltype(std::as_const(ry))>);

    // Note: 无法对 右值，as_const
    // printValue(std::as_const(int{2}));

    // Note: 如果是模板呢？
    auto fun = []<typename T>(T &&data) {
        // printValue(std::as_const(int{2})); //Note: 无法对右值，as_const的
        printValue(std::as_const(data));
    };
    fun(int{2}); // OK
    fun(std::move(x));

    // 测试函数
    // Note: 外部const 修饰是 限制很大的
    // printValue(std::as_const(test())); // 不允许，明显很不灵活

    return 0;
}