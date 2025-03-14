#include <iostream>
#include <type_traits>

template <typename T>
struct has_copy_constructor
{
    static constexpr bool value = std::is_constructible_v<T, const T &>;
};

template <typename T>
struct has_move_constructor
{
    static constexpr bool value = std::is_constructible_v<T, T &&>;
};

template <typename T>
struct has_copy_or_move_constructor
{
    static constexpr bool value =
        std::is_constructible_v<T, const T &> || std::is_constructible_v<T, T &&>;
};

struct NoCopy
{
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete; // 删除复制构造函数
    NoCopy(NoCopy &&) = default;     // 保留移动构造函数
};

struct NoMove
{
    NoMove() = default;
    NoMove(const NoMove &) = default; // 保留复制构造函数
    NoMove(NoMove &&) = delete;       // 删除移动构造函数
};

struct NoCopyNoMove
{
    NoCopyNoMove() = default;
    NoCopyNoMove(const NoCopyNoMove &) = delete; // 删除复制构造函数
    NoCopyNoMove(NoCopyNoMove &&) = delete;      // 删除移动构造函数
};

int main()
{
    std::cout << std::boolalpha;
    std::cout << "NoCopy has copy constructor: " << has_copy_constructor<NoCopy>::value
              << "\n"; // false
    std::cout << "NoCopy has move constructor: " << has_move_constructor<NoCopy>::value
              << "\n"; // true
    std::cout << "NoMove has copy constructor: " << has_copy_constructor<NoMove>::value
              << "\n"; // true
    std::cout << "NoMove has move constructor: " << has_move_constructor<NoMove>::value
              << "\n"; // false
    std::cout << "NoCopyNoMove has copy or move constructor: "
              << has_copy_or_move_constructor<NoCopyNoMove>::value << "\n"; // false
    return 0;
}