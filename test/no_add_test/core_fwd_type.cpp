#include <iostream>
#include <type_traits>
#include <utility>

// NOLINTBEGIN
// 测试函数：检查传入参数的推导类型是否与预期一致
template <typename Expect, typename T>
void test_type(T &&t)
    requires(std::is_same_v<Expect, decltype(t)>)
{
    (void)t; // 消除未使用变量警告
}

// 可拷贝可移动类型
struct CopyMoveType
{
    CopyMoveType() = default;
    CopyMoveType(const CopyMoveType &) = default;
    CopyMoveType &operator=(const CopyMoveType &) = default;
    CopyMoveType(CopyMoveType &&) noexcept = default;
    CopyMoveType &operator=(CopyMoveType &&) noexcept = default;
};

// 仅可移动类型
struct MoveOnlyType
{
    MoveOnlyType() = default;
    MoveOnlyType(const MoveOnlyType &) = delete;
    MoveOnlyType &operator=(const MoveOnlyType &) = delete;
    MoveOnlyType(MoveOnlyType &&) noexcept = default;
    MoveOnlyType &operator=(MoveOnlyType &&) noexcept = default;
};

int main()
{
    // 测试基本类型（int）
    int int_lvalue = 42;
    test_type<int &>(int_lvalue);             // 左值 → 左值引用（int&）
    test_type<int &&>(5);                     // 纯右值 → 右值引用（int&&）
    test_type<int &&>(std::move(int_lvalue)); // 将亡值 → 右值引用（int&&）

    // 测试可拷贝可移动类型
    CopyMoveType cm_lvalue;
    test_type<CopyMoveType &>(cm_lvalue);       // 左值 → 左值引用（CopyMoveType&）
    test_type<CopyMoveType &&>(CopyMoveType{}); // 纯右值 → 右值引用（CopyMoveType&&）
    test_type<CopyMoveType &&>(
        std::move(cm_lvalue)); // 将亡值 → 右值引用（CopyMoveType&&）

    // 测试仅可移动类型
    MoveOnlyType mo_lvalue;
    test_type<MoveOnlyType &>(mo_lvalue);       // 左值 → 左值引用（MoveOnlyType&）
    test_type<MoveOnlyType &&>(MoveOnlyType{}); // 纯右值 → 右值引用（MoveOnlyType&&）
    test_type<MoveOnlyType &&>(
        std::move(mo_lvalue)); // 将亡值 → 右值引用（MoveOnlyType&&）

    // 添加 volatile 和 const 的部分
    // 基本类型 + const
    const int const_int_lvalue = 43;
    test_type<const int &>(const_int_lvalue);             // const左值 → const int&
    test_type<const int &&>(std::move(const_int_lvalue)); // const将亡值 → const int&&
    test_type<int &&>(
        static_cast<const int>(int{6})); // const纯右值 → const int&& ->int&&

    // 基本类型 + volatile
    volatile int vol_int_lvalue = 44;
    test_type<volatile int &>(vol_int_lvalue); // volatile左值 → volatile int&
    test_type<volatile int &&>(
        std::move(vol_int_lvalue));                  // volatile将亡值 → volatile int&&
    test_type<int &&>(static_cast<volatile int>(7)); // volatile纯右值 →  int&&
    test_type<int &&>(static_cast<volatile int>(7));

    // 基本类型 + const volatile
    const volatile int cv_int_lvalue = 45;
    test_type<const volatile int &>(
        cv_int_lvalue); // const volatile左值 → const volatile int&
    test_type<const volatile int &&>(
        std::move(cv_int_lvalue)); // const volatile将亡值 → const volatile int&&
    test_type<int &&>(static_cast<const volatile int>(
        8)); // const volatile纯右值 → const volatile int&&

    // NOTE: 基本类型纯右值，还是纯右值，不带cv。 完美转发，依然奏效。

    // NOTE: 基本类型 和 用户类型，还是有点差别的
    //  类类型的纯右值 cv 限定符会保留（与基本类型不同）
    test_type<const CopyMoveType &&>(static_cast<const CopyMoveType>(
        CopyMoveType{})); // 类类型const纯右值 →
                          // 保留const，推导为const CopyMoveType&&
    test_type<volatile MoveOnlyType &&>(static_cast<volatile MoveOnlyType>(
        MoveOnlyType{})); // 类类型volatile纯右值 → 保留volatile，推导为volatile
                          // MoveOnlyType&&

    // 类类型纯右值：cv限定符通过static_cast保留
    // 可拷贝可移动类型 + const
    test_type<const CopyMoveType &&>(static_cast<const CopyMoveType>(
        CopyMoveType{}) // 显式转换为const纯右值 → 保留const
    );
    // 可拷贝可移动类型 + volatile
    test_type<volatile CopyMoveType &&>(static_cast<volatile CopyMoveType>(
        CopyMoveType{}) // 显式转换为volatile纯右值 → 保留volatile
    );

    // 仅可移动类型 + const
    test_type<const MoveOnlyType &&>(static_cast<const MoveOnlyType>(
        MoveOnlyType{}) // 显式转换为const纯右值 → 保留const
    );
    // 仅可移动类型 + volatile
    test_type<volatile MoveOnlyType &&>(static_cast<volatile MoveOnlyType>(
        MoveOnlyType{}) // 显式转换为volatile纯右值 → 保留volatile
    );

    // 类类型纯右值 + const volatile（双重限定）
    test_type<const volatile CopyMoveType &&>(
        static_cast<const volatile CopyMoveType>(CopyMoveType{}) // 保留双重限定
    );

    std::cout << "所有静态断言通过！\n";
    return 0;
}
// NOLINTEND
