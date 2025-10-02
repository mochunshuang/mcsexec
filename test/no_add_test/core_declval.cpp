#include <iostream>
#include <utility>
// NOLINTBEGIN
#include <utility> // for std::move

// 可复制可移动类型
struct CopyMoveType
{
    // 默认构造函数
    CopyMoveType() = default;

    // 拷贝构造函数（允许复制）
    CopyMoveType(const CopyMoveType &)
    {
        // 实际中会复制资源（如内存、文件句柄等）
    }

    // 拷贝赋值运算符（允许复制赋值）
    CopyMoveType &operator=(const CopyMoveType &)
    {
        // 复制资源
        return *this;
    }

    // 移动构造函数（允许移动）
    CopyMoveType(CopyMoveType &&) noexcept
    {
        // 实际中会转移资源所有权（不复制，仅接管）
    }

    // 移动赋值运算符（允许移动赋值）
    CopyMoveType &operator=(CopyMoveType &&) noexcept
    {
        // 转移资源所有权
        return *this;
    }
};
// 仅可移动类型
struct MoveOnlyType
{
    // 默认构造函数
    MoveOnlyType() = default;

    // 禁止拷贝构造（核心：删除拷贝操作）
    MoveOnlyType(const MoveOnlyType &) = delete;

    // 禁止拷贝赋值
    MoveOnlyType &operator=(const MoveOnlyType &) = delete;

    // 允许移动构造
    MoveOnlyType(MoveOnlyType &&) noexcept
    {
        // 转移资源所有权
    }

    // 允许移动赋值
    MoveOnlyType &operator=(MoveOnlyType &&) noexcept
    {
        // 转移资源所有权
        return *this;
    }
};
// NOLINTEND
int main()
{
    /*
    _EXPORT_STD template <class _Ty>
    add_rvalue_reference_t<_Ty> declval() noexcept {
        static_assert(false, "Calling declval is ill-formed, see N4950 [declval]/2.");
    }
    */
    using T = decltype(std::declval<int>());
    static_assert(std::is_same_v<T, int &&>);

    using T0 = decltype(std::declval<int &>());
    static_assert(std::is_same_v<T0, int &>);

    using T1 = decltype(std::declval<int &&>());
    static_assert(std::is_same_v<T1, int &&>);

    // 测试 CopyMoveType
    using CMT_RValue =
        decltype(std::declval<CopyMoveType>()); // 推导为 CopyMoveType&&（右值引用）
    using CMT_LValue =
        decltype(std::declval<CopyMoveType &>()); // 推导为 CopyMoveType&（左值引用）

    // 测试 MoveOnlyType
    using MOT_RValue =
        decltype(std::declval<MoveOnlyType>()); // 推导为 MoveOnlyType&&（右值引用）
    using MOT_LValue =
        decltype(std::declval<MoveOnlyType &>()); // 推导为 MoveOnlyType&（左值引用）

    // 验证推导结果
    static_assert(std::is_same_v<CMT_RValue, CopyMoveType &&>);
    static_assert(std::is_same_v<CMT_LValue, CopyMoveType &>);
    static_assert(std::is_same_v<MOT_RValue, MoveOnlyType &&>);
    static_assert(std::is_same_v<MOT_LValue, MoveOnlyType &>);

    // NOTE: 结论没有任何区别。 不关心 移动 或 复制 的构造是否delete. 仅仅类型转换

    std::cout << "main done\n";
    return 0;
}