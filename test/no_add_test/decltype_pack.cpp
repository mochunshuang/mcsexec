
#include <type_traits>

// 假设的 get_completion_signatures 模板
template <typename T>
struct get_completion_signatures
{
    static constexpr int value = 1; // NOLINT
};

// 测试模板
template <typename... Sndrs>
void test() // NOLINT
{
    // NOTE: 使用 decltype 和包展开,需要加括号
    // using Sigs = decltype(get_completion_signatures<Sndrs>::value + ... + 0);
    // 使用折叠表达式
    using Sigs = decltype((get_completion_signatures<Sndrs>::value + ... + 0));

    // 检查 Sigs 的类型
    static_assert(std::is_same_v<Sigs, int>);
}

int main()
{
    // 测试
    test<int, double, char>();

    return 0;
}