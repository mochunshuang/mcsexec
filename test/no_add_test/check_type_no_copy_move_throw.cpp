#include <utility>
#include <iostream>
#include <type_traits>

// NOTE: 没有短路，不存在还会继续判断
#if 0
template <typename Tag, typename... T>
struct check_type_no_copy_move_throw<Tag(T...)>
{

    static constexpr bool value = // NOLINT
        (((std::is_copy_constructible_v<T> && noexcept(T(std::declval<const T &>()))) &&
          (std::is_move_constructible_v<T> && noexcept(T(std::declval<T &&>())))) &&
         ...);
};
#endif

// 辅助模板：检查复制构造是否noexcept（仅在存在复制构造函数时检查）
template <typename T, bool = std::is_copy_constructible_v<T>>
struct check_copy_noexcept : std::false_type
{
};

template <typename T>
struct check_copy_noexcept<T, true>
    : std::integral_constant<bool, noexcept(T(std::declval<const T &>()))>
{
};

// 辅助模板：检查移动构造是否noexcept（仅在存在移动构造函数时检查）
template <typename T, bool = std::is_move_constructible_v<T>>
struct check_move_noexcept : std::false_type
{
};

template <typename T>
struct check_move_noexcept<T, true>
    : std::integral_constant<bool, noexcept(T(std::declval<T &&>()))>
{
};

// 主模板
template <typename Sigs>
struct check_type_no_copy_move_throw;

template <typename Tag, typename... T>
struct check_type_no_copy_move_throw<Tag(T...)>
{
    static constexpr bool value =
        ((check_copy_noexcept<T>::value && check_move_noexcept<T>::value) && ...);
};

// 测试类型定义
struct NoCopy
{
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete;      // 删除复制构造函数
    NoCopy(NoCopy &&) noexcept = default; // 移动构造函数标记为noexcept
};

struct NoMove
{
    NoMove() = default;
    NoMove(const NoMove &) noexcept = default; // 复制构造函数标记为noexcept
    NoMove(NoMove &&) = delete;                // 删除移动构造函数
};

struct NoThrow
{
    NoThrow() = default;
    NoThrow(const NoThrow &) noexcept = default; // 复制构造函数noexcept
    NoThrow(NoThrow &&) noexcept = default;      // 移动构造函数noexcept
};

int main()
{
    std::cout << std::boolalpha;
    std::cout << "NoCopy: " << check_type_no_copy_move_throw<void(NoCopy)>::value
              << "\n"; // false
    std::cout << "NoMove: " << check_type_no_copy_move_throw<void(NoMove)>::value
              << "\n"; // false
    std::cout << "NoThrow: " << check_type_no_copy_move_throw<void(NoThrow)>::value
              << "\n"; // true
    return 0;
}