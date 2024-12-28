#include <type_traits>
#include <concepts>

// 概念：检查单个类型是否可能在 decay-copying 过程中抛出异常
template <typename T>
concept single_potentially_throwing =
    (std::is_constructible_v<T> && !std::is_nothrow_default_constructible_v<T>) ||
    (std::is_copy_constructible_v<T> && !std::is_nothrow_copy_constructible_v<T>) ||
    (std::is_move_constructible_v<T> && !std::is_nothrow_move_constructible_v<T>) ||
    (std::is_copy_assignable_v<T> && !std::is_nothrow_copy_assignable_v<T>) ||
    (std::is_move_assignable_v<T> && !std::is_nothrow_move_assignable_v<T>);

// 概念：检查多个类型是否可能在 decay-copying 过程中抛出异常
template <typename... Ts>
concept potentially_throwing = (single_potentially_throwing<Ts> || ...);

// 示例类
class MyClass // NOLINT
{
  public:
    MyClass() noexcept = default;                           // 默认构造函数
    MyClass(const MyClass &) noexcept = default;            // 复制构造函数
    MyClass(MyClass &&) noexcept = default;                 // 移动构造函数
    MyClass &operator=(const MyClass &) noexcept = default; // 复制赋值运算符
    MyClass &operator=(MyClass &&) noexcept = default;      // 移动赋值运算符
};

class MyThrowClass // NOLINT
{
  public:
    MyThrowClass() noexcept(false) {} // 可能抛出异常的默认构造函数
    MyThrowClass(const MyThrowClass &) noexcept = default; // 复制构造函数
    MyThrowClass(MyThrowClass &&) noexcept = default;      // 移动构造函数
    MyThrowClass &operator=(const MyThrowClass &) noexcept = default; // 复制赋值运算符
    MyThrowClass &operator=(MyThrowClass &&) noexcept = default; // 移动赋值运算符
};

class MyDeletedClass // NOLINT
{
  public:
    MyDeletedClass() = delete;                       // 删除默认构造函数
    MyDeletedClass(const MyDeletedClass &) = delete; // 删除复制构造函数
    MyDeletedClass(MyDeletedClass &&) = delete;      // 删除移动构造函数
    MyDeletedClass &operator=(const MyDeletedClass &) = delete; // 删除复制赋值运算符
    MyDeletedClass &operator=(MyDeletedClass &&) = delete; // 删除移动赋值运算符
};

int main()
{
    static_assert(not potentially_throwing<MyClass>,
                  "MyClass should not potentially throw");
    static_assert(potentially_throwing<MyThrowClass>,
                  "MyThrowClass should potentially throw");
    static_assert(not potentially_throwing<MyDeletedClass>,
                  "MyDeletedClass should not potentially throw");

    static_assert(potentially_throwing<MyClass, MyThrowClass>,
                  "MyClass and MyThrowClass should potentially throw");
    static_assert(potentially_throwing<MyClass, MyThrowClass, MyDeletedClass>,
                  "MyClass, MyThrowClass, and MyDeletedClass should potentially throw");
}