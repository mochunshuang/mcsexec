
// 编译期表达式合法性检测接口
template <auto F>
concept valid_expr = requires { F(); };

// 测试合法表达式
constexpr int valid_func() // NOLINT
{
    return 2;
}
static_assert(valid_expr<valid_func>); // ✅ 合法表达式通过

// 测试非法表达式（含 throw 的 consteval）

[[noreturn]] consteval int invalid_func() // NOLINT
{
    throw 2; // 虽然语法合法，但编译期无法执行 // NOLINT
}

// 做不到哦
// TODO(mcs): 等待c++26，捕获编译期异常才可以
// static_assert(!valid_expr<invalid_func>);

[[noreturn]] consteval int invalid_func2() // NOLINT
    ;
// 用未定义，来替代也是语法错误。在包装valid_expr概念能做到吗
// static_assert(!valid_expr<invalid_func2>);

int main()
{
    return 0;
}