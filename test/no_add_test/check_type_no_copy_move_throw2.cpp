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

// NOTE: 不存在 返回 true
//  检查复制构造是否noexcept（短路逻辑）
template <typename T>
concept copy_no_except = not std::is_copy_constructible_v<T> ||
                         (std::is_copy_constructible_v<T> && requires(const T &t) {
                             { T(t) } noexcept;
                         });

// 检查移动构造是否noexcept（短路逻辑）
template <typename T>
concept move_no_except = not std::is_move_constructible_v<T> ||
                         (std::is_move_constructible_v<T> && requires(T &&t) {
                             { T(std::move(t)) } noexcept;
                         });

// TODO(mcs): ，修改以下逻辑
// 总约束：如果移动存在，框架优先使用移动 传递消息，因此不考虑复制
// 情况一：移动存在，判断移动是否存在异常，作为结果
// 情况二：移动不存在，判断复制是否存在异常，作为结果
// 情况三：移动和复制都不存在，no_copy_move_throw 是 true。 可能编译期都过不了，不用管
template <typename... Ts>
concept no_copy_move_throw = ((copy_no_except<Ts> && move_no_except<Ts>) && ...);

// 主模板
template <typename Sigs>
struct check_type_no_copy_move_throw;

template <typename Tag, typename... T>
struct check_type_no_copy_move_throw<Tag(T...)>
{
    static constexpr bool value = no_copy_move_throw<T...>;
};

// 测试类型
struct NoCopy
{
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete;      // 删除复制构造
    NoCopy(NoCopy &&) noexcept = default; // 移动构造noexcept
};

struct NoMove
{
    NoMove() = default;
    NoMove(const NoMove &) noexcept = default;
    NoMove(NoMove &&) = delete; // 删除移动构造
};

struct NoThrow
{
    NoThrow() = default;
    NoThrow(const NoThrow &) noexcept = default;
    NoThrow(NoThrow &&) noexcept = default;
};

struct DeleteAll // NOLINT
{
    DeleteAll() = default;
    DeleteAll(const DeleteAll &) noexcept = delete;
    DeleteAll(DeleteAll &&) noexcept = delete;
};

struct THROW
{
    THROW() = default;
    THROW(const THROW &) {};
    THROW(THROW &&) {};
};

struct NO_THROW
{
    NO_THROW() = default;
    NO_THROW(const NO_THROW &) {}; // 有异常但是不应该给被校验到
    NO_THROW(NO_THROW &&) = default;
};

struct NO_THROW2
{
    NO_THROW2() = default;
    NO_THROW2(const NO_THROW2 &) noexcept {};
    NO_THROW2(NO_THROW2 &&) = delete;
};
// NOLINT

// 总约束：如果移动存在，优先使用移动；否则使用复制
template <typename T>
concept no_copy_move_throw2 =
    (std::is_move_constructible_v<T> && move_no_except<T>) ||
    (!std::is_move_constructible_v<T> && copy_no_except<T>) ||
    (!std::is_move_constructible_v<T> && !std::is_copy_constructible_v<T>);

// 主模板
template <typename Sigs>
struct check_type_no_copy_move_throw2;

template <typename Tag, typename... T>
struct check_type_no_copy_move_throw2<Tag(T...)>
{
    static constexpr bool value = (no_copy_move_throw2<T> && ...);
};

int main()
{
    std::cout << std::boolalpha;
    std::cout << "NoCopy: " << check_type_no_copy_move_throw<void(NoCopy)>::value
              << "\n"; // true
    std::cout << "NoMove: " << check_type_no_copy_move_throw<void(NoMove)>::value
              << "\n"; // true
    std::cout << "NoThrow: " << check_type_no_copy_move_throw<void(NoThrow)>::value
              << "\n"; // true

    std::cout << "DeleteAll: " << check_type_no_copy_move_throw<void(DeleteAll)>::value
              << "\n"; // true

    std::cout << "THROW: " << check_type_no_copy_move_throw<void(THROW)>::value
              << "\n"; // false

    // 目前是false ，请修改它
    std::cout << "NO_THROW: " << check_type_no_copy_move_throw<void(NO_THROW)>::value
              << "\n"; // false

    // NOTE: 现在满足了
    std::cout << "NO_THROW: " << check_type_no_copy_move_throw2<void(NO_THROW)>::value
              << "\n"; // true
    {
        static_assert(check_type_no_copy_move_throw2<void(NO_THROW)>::value);
        // NOLINT
        static_assert(check_type_no_copy_move_throw2<void(NoCopy)>::value);
        static_assert(check_type_no_copy_move_throw2<void(NoMove)>::value);
        static_assert(check_type_no_copy_move_throw2<void(NoThrow)>::value);
        static_assert(check_type_no_copy_move_throw2<void(DeleteAll)>::value);
        static_assert(not check_type_no_copy_move_throw2<void(THROW)>::value);
        // NOLINT

        static_assert(check_type_no_copy_move_throw2<void(NO_THROW2)>::value);
    }
}